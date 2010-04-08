SQLSTATE_MAPPING_REQUIRED

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erlq.msg -	Message file for class LQ
**
** Description:
**	This file defines the messages for class LQ. 
**      This includes LIBQ and LIBQXA (an optional library, for DTP/XA support)
**
** History:
**	01-sep-1987 (barbara)
**		Created.
**	08-jul-1988 (neil)
**		Added comments to all errors.
**	06-jun-1989 (barbara)
**		Added multiple connection errors.
**	23-mar-1990 (barbara)
**		Added new multiple connection msg for bug 9157.
**	03-mar-1990 (barbara)
**		Added new nested looping errors in mult conns for bug 9159.
**	08-may-1990 (neil)
**		Added PFLOAD error.
**	15-jun-1990 (barbara)
**		Added 2D and 2E association fail errors for bug fix 21832.
**	23-oct-1990 (mgw)
**		Corrected a misspelling. It was in a comment, not a message,
**		but this comment gets picked up for the messages.txt file via
**		erxtract and is thus in a deliverable.
**	01-apr-1991 (kathryn)
**		Added message E_LQ00A4_ATTVALTYPE.
**	10-apr-1991 (kathryn)
**		Added message E_LQ00A5_TRACEFILE.
**	20-jul-1991 (teresal)
**		Changed EVENT to DBEVENT.
**      01-oct-1992 (mani)
**              Started adding error msgs for Ingres65 XA support. These will
**              run from E_LQ00D0 to ELQ00DF.
**	01-nov-1992 (larrym)
**		Added message E_LQ0045_SQLSTALLOC
**	01-nov-1992 (kathryn)
**		Added the following messages for large object handling:
**		E_LQ00E0_NODATAHDLR    E_LQ00E1_DATHDLRTYPE 
**	 	E_LQ00E2_GETHDLRONLY   E_LQ00E3_GETNOLEN      
**		E_LQ00E4_NOENDDATA     E_LQ00E5_PUTHDLRONLY
**		E_LQ00E6_PUTNOLEN      E_LQ00E7_NODATAEND
**      02-nov-1992 (larrym)
**		removed message E_LQ0045_SQLSTALLOC.
**              Added message E_LQ0045_DA_INV_ITEM_TYPE
**	18-nov-1992 (larrym)
**		Added E_LQ000F_SESLIM
**	15-dec-1992 (larrym)
**	    Added E_LQ00AD - E_LQ00AF, E_LQ00B2 - E_LQ00B9 for DB 
**	    procedure BYREF param support
**	19-jan-1993 (teresal)
**	    Added E_LQ00E8_ADFALLOC for multiple ADF control blocks.
**	    This is part of the decimal changes.
**	15-jun-1993 (larrym)
**	    Added new XA error messages 00D5 thru 00D7
**	01-oct-1993 (kathryn)
**	    Add E_LQ00E9_BADSEGLEN for PUT DATA statement.
**	    Changed 'long varchar' references to 'long' datatypes.
**	4-nov-93 (robf)
**          Add E_LQ00EA_PROMPT_ALLOC, E_LQ00EB_PROMPT_PROTOCOL.
**	09-Nov-1993 (larrym)
**	    Added E_LQ00D8 and E_LQ00D9.  Also fixed SQLSTATE for E_LQ00A2.
**	06-Jan-1993 (teresal)
**	    Added error message for non-terminated C string (FIPS).
**	    Bug fix 55915. Added SQLSTATE 22024.
**	28-jan-1994 (larrym)
**	    Added E_LQ00DA, for authorization string checking.
**	19-oct-96 (mcgem01)
**	    References to MISC_ERRORS changed to MISC_ERRORS to
**	    facilitate code-sharing with Jasmine.
**	18-Dec-97 (gordy)
**	    Added E_LQ00C8_TLS_ALLOC, E_LQ00C9_SESS_ACTIVE,
**	    E_LQ00CA_SESS_ACTIVE, E_LQ00CB_SESS_ACTIVE for
**	    multi-threaded application support.
**	2-may-01 (inkdo01)
**	    Change text/description for LQ003C & LQ006A for row producing 
**	    procs, as well as select loops.
**	08-nov-02 (drivi01)
**	    Added a new error message for 
**	    W_LQ00ED_ING_SET_USAGE:SS50008_UNSUPPORTED_STMT error.
**       3-Apr-2008 (hanal04) Bug 120202
**          Corrected typo in E_LQ00E4.
**/

# define	_LQ_CLASS	151



E_LQ0001_STARTUP:SS08001_CANT_GET_CONNECTION	
"Failed to connect to DBMS session."

/*
** Explanation:
**	The attempt to connect to a DBMS failed.  Prior to this error
**	message there should have been another error message displayed.
**	This message is only displayed if the failure was not due to
**	obvious reasons, such as non-existent database, wrong permissions,
**	or an invalid command-line flag.  This message will usually be
**	displayed if the communications system failed.
**
** System Status:
**
** User Action:
*/

E_LQ0002_USTARTUP:SS08001_CANT_GET_CONNECTION	
"Failure to connect to DBMS.  Operating system error %0c."

/*
** Explanation:
**	Under certain circumstances the failure to connect to a DBMS is due
**	to operation system reasons.  For example lack of process resources
**	or quotas.  If this is the reason, then an attempt will be made to 
**	display the system error message text so that the system administrator
**	or DBA can be informed.
**
** System Status:
**
** User Action:
*/

E_LQ0003_ADFINIT:SS08001_CANT_GET_CONNECTION	
"System unsuccessful in initializing Abstract Data-Type Facility.  Exiting. . ."

/*
** Explanation:
**	When connecting to a DBMS the above facility (ADF) must be initialized
**	in order to process queries and data values.  If the facility
**	could not be initialized then it will display an error message.
**	Upon return the above message will be displayed and the connection
**	will not be made and the application will terminate.
**
** System Status:
**
** User Action:
*/

E_LQ0004_TWODB:SS08001_CANT_GET_CONNECTION	
"Attempt to connect to DBMS twice in one process."

/*
** Explanation:
**	You are not allowed to connect to 2 INGRES databases at the same
**	time in one process.  You must exit one database in order to start
**	up another (or the same one again).
**
** System Status:
**
** User Action:
*/

E_LQ0005_FLAGS:SS08001_CANT_GET_CONNECTION	
"DBMS connection flag '%0c' is no longer supported."

/*
** Explanation:
**	The specified connection startup flag was supported in previous
**	versions of INGRES but is no longer supported.  You should modify
**	your application, or your command line, so that it does not issue
**	this startup flag any more.
**
** System Status:
**
** User Action:
*/

E_LQ0006_ASSOC_BAD_FLAG:SS08001_CANT_GET_CONNECTION		
"Unknown or null DBMS connection startup argument %0c."

/* The above message is used by libqgca */
/*
** Explanation:
**	Each startup argument must correspond to a legal INGRES startup flag
**	as specified in the INGRES manual.  The specified argument is not a
**	legal flag and should be removed from the application or command
**	line.  This message is also issued if no database name (or a null
**	database name) is specified.
**
** System Status:
**
** User Action:
*/

E_LQ0007_PRINTQRY:SS50000_MISC_ERRORS	
"Failed to open '%0c' file in current directory.\n  
Unable to provide internal query trace information."

/*
** Explanation:
**	When assigning the value "printqry" or "printgca" to the symbol
**	II_EMBED_SET, the run-time system records all your queries or
**	communication message into the specified file in the current
**	directory.  For certain reasons (such as permissions or quotas)
**	the trace file could not be opened.  This will not have any effect
**	on the processing of your queries.
**
** System Status:
**
** User Action:
*/

E_LQ0008_XID:SS08001_CANT_GET_CONNECTION	
"Syntax error in WITH clause on CONNECT statement.\n
The distributed transaction ids are incorrectly specified."

/*
** Explanation:
**	When reconnecting after a broken connection during two-phase
**	commit, the distributed transaction id must be the only
**	component of a WITH clause.  Furthermore the HIGHDXID
**	must be specified before the LOWDXID.
**
** System Status:
**
** User Action:
*/

E_LQ0009_GCFMT:SS5000L_PROTOCOL_ERROR	
"Communications or transmission error received without text.\n
The generic error number is %0c, the local error is %1c, and the number of
parameters is %2c (should be 1)."

/*
** Explanation:
**	A GCA communications error has occurred, but the error format
**	(incorrectly) included no text.  
**
** System Status:
**
** User Action:
*/

E_LQ000A_EMBBAD:SS22005_ASSIGNMENT_ERROR	
"Conversion failure from column %0c into program variable."

/*
** Explanation:
**	When retrieving data from the database, the data being assigned into
**	your result variable could not be converted to the correct type.
**	This could be due incompatibilities between types, lengths or other
**	data type issues.  The parameter specifies which column number in
**	the retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ000B_EMBOVFL:SS22003_NUM_VAL_OUT_OF_RANGE	
"Overflow of numeric data from column %0c into program variable."

/*
** Explanation:
**	When retrieving data from the database, the numeric data being
**	assigned into your result variable caused an overflow condition.
**	You should provide a larger result variable.  The parameter specifies
**	which column number in the retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ000C_EMBCNV:SS22005_ASSIGNMENT_ERROR	
"Column %0c can not be converted into program variable.\n
Data types are incompatible."

/*
** Explanation:
**	When retrieving data from the database, the data being assigned into
**	your result variable was incompatible with the data type of the
**	result variable.  You should modify the data type of the result so
**	that it can retrieve the data.  A general rule states that all
**	numerics are compatible with each-other, as are all strings.  
**	Strings and numerics are not compatible. The parameter specifies
**	which column number in the retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ000D_EMBDTID:SS22500_INVALID_DATA_TYPE	
"Column %0c can not be converted into program variable.\n
Unrecognized internal data type in conversion routine."

/*
** Explanation:
**	When retrieving data from the database, the data could not be assigned
**	into your result variable as the data type of the variable was invalid.
**	You should reissue the retrieval (and rebuild the application) to
**	confirm that this error is not due to a coding error.
**	The parameter specifies which column number in the retrieval the
**	error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ000E_EMBIND:SS22002_NULLVAL_NO_IND_PARAM	
"No null indicator supplied with program variable.\n
Null data from column %0c can not be converted."

/*
** Explanation:
**	When retrieving data from the database, the NULL data could not be
**	assigned into your result variable as there was no null indicator
**	provided.  Nullable data can be assigned to a variable that does
**	not have a null indicator, though if the data is NULL then this error
**	will be issued.  The parameter specifies which column number in the
**	retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ000F_SESLIM:SS5000L_PROTOCOL_ERROR
"Exceded maximum number of sessions."

/*
** Explanation:
**	The application has requested that a session ID be generated for it,
**	and an ID small enough to fit into a four byte integer was not found.
**
** System Status:
**	The connection will not be established.
**
** User Action:
**	In order for this error to occur, the application would have to have
**	exceeded 2^32 sessions.   This error is more likely the result
**	of an internal coding error.   Please report this problem to your 
**	technical support representative.
*/

E_LQ0010_EMBNULLIN:SS22023_INV_PARAM_VAL	
"Null address of program variable in input statement.\n
Variable cannot be used to set values."

/*
** Explanation:
**	When setting data in the database from a program variable, the variable
**	had a zero address.   Consequently, no data could be assigned into
**	the database.
**
** System Status:
**
** User Action:
*/

E_LQ0011_EMBNULLOUT:SS22005_ASSIGNMENT_ERROR	
"Null address of program variable in output statement.\n
Data cannot be retrieved from column %0c into variable."

/*
** Explanation:
**	When retrieving data from the database, the data could not be
**	assigned into your result variable as there was a zero result
**	variable data address.  The parameter specifies which column number
**	in the retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ0012_DBTOOLONG:SS22005_ASSIGNMENT_ERROR	
"Length of result data (%0c) is too long for processing.\n
Data cannot be retrieved from column %1c into variable."

/*
** Explanation:
**	When retrieving data from the database, the data could not be
**	assigned into your result variable as the internal data was too
**	long for processing.  If the data to be retrieved is longer than
**	your result variable, then the communications interface uses a 
**	temporary buffer to receive that data.  This temporary buffer was
**	not long enough.  The parameter specifies which column number
**	in the retrieval the error occurred.
**
** System Status:
**
** User Action:
*/

E_LQ0014_DBVAR:SS22500_INVALID_DATA_TYPE	
"Invalid type or length describing program data.\n
Unable to convert into internal format."

/*
** Explanation:
**	When setting data in the database from a program variable, the
**	variable had an invalid type or length associated with it.
**	Consequently, no data could be assigned into the database.  You 
**	should reissue the statement (and rebuild the application) to
**	confirm that this error is not due to a coding error.
**
** System Status:
**
** User Action:
*/

E_LQ0015_DBOVF:SS22003_NUM_VAL_OUT_OF_RANGE	
"Overflow on converting numeric program data into internal format."

/*
** Explanation:
**	When setting data in the database from a program variable, an overflow
**	error occurred while converting from a program variable to an internal
**	representation.  Consequently, no data could be assigned into the
**	database.  
**
** System Status:
**
** User Action:
*/

E_LQ0016_DBNULL:SS22000_DATA_EXCEPTION	
"Unable to convert null program data.\n
Internal format is non-nullable."

/*
** Explanation:
**	When setting data in the database from a program variable, a conversion
**	error occurred while converting from a program variable with a null
**	indicator (which is set) to an internal non-nullable representation.
**	Consequently, no data could be assigned into the database.  
**
** System Status:
**
** User Action:
*/

E_LQ0017_DBCONV:SS22000_DATA_EXCEPTION	
"Conversion failure from program data into internal format."

/*
** Explanation:
**	When setting data in the database from a program variable, a
**	conversion error occurred.  This could be due incompatibilities
**	between types, lengths or other data type issues.  
**
**
** System Status:
**
** User Action:
*/

E_LQ0018_DBQRY_NULL:SS22000_DATA_EXCEPTION 
"Run-time component of query is NULL valued.\n
Variables or fields used in parts of queries must not be NULL."

/*
** Explanation:
**	When using variables or fields to replace parts of a query you must
**	not use the NULL value.  This can occur when using a 4GL field to
**	replace a query component.  Replace the field with a valid syntactic
**	value.
**
** System Status:
**	Query is ignored.
**
** User Action:
**	Replace the field or variable with a valid query component.
*/


E_LQ001F_BADPARAM:SS22023_INV_PARAM_VAL	
"Invalid type (%0c) specified in target list of parameterized statement."

/*
** Explanation:
**	When using EQUEL PARAM target lists the format specification is
**	invalid.  Note that no spaces are allowed within format
**	specifications.  For example, "% i4" is wrong - "%i4" is correct.
**
** System Status:
**
** User Action:
*/

E_LQ0020_PARMMAX:SS21000_CARD_VIOLATION    
"Too many result columns (%0c) in PARAM target list.\n
The number used exceeds the maximum number of domains."

/*
** Explanation:
**	The number of result columns of an EQUEL RETRIEVE PARAM statement
**	must not exceed 127.
**
** System Status:
**
** User Action:
*/


E_LQ0021_RETPARAM:SS50003_EXHAUSTED_RESOURCE	
"No internal descriptor for PARAM variables.\n
Memory allocation failed."

/*
** Explanation:
**	When trying to execute an EQUEL RETRIEVE PARAM statement, the
**	descriptor required to process result values with the addresses
**	specified in the PARAM target list could not be allocated due
**	to memory shortage.
**
** System Status:
**
** User Action:
*/

E_LQ002A_NODB:SS08003_NO_CONNECTION	
"The '%0c' query has been issued outside of a DBMS session.\n
Exiting . . ."

/*
** Explanation:
**	The specified query was issued without any connection to a DBMS.
**	This error may occur if you either did not issue any CONNECT or
**	INGRES statement, or if the connection failed and the application
**	continued.  If you are running an Embedded SQL application, make sure
**	you allow the printing of errors as previous errors may help
**	solve the source of this error message.
**
** System Status:
**	The program will exit.
**
** User Action:
*/

E_LQ002B_INQUERY:SS25000_INV_XACT_STATE	
"'%0c' may not be nested within a data retrieval loop."

/*
** Explanation:
**	No database statements may be issued within a SELECT/RETRIEVE
**	loop.  In order to terminate a data retrieval loop, you must either
**	let it run to completion or issue an ENDSELECT/ENDRETRIEVE statement.
**	
** System Status:
**
** User Action:
*/

E_LQ002C_INPROC:SS25000_INV_XACT_STATE	
"'%0c' may not be issued while a database procedure is being processed."

/*
** Explanation:
**	The specified query was issued during the processing of a database
**	procedure.  Even if the procedure could not be invoked, and this
**	caused an error handled by the calling application, you may
**	not issue any database statements until database procedure
**	processing is complete.  When control is returned to the run-time
**	system, the procedure processing will be terminated.  A database
**	procedure must either run to completion or be terminated
**	via the WHENEVER GOTO action for Embedded SQL error handling.
**
** System Status:
**
** User Action:
*/

E_LQ002D_ASSOCFAIL:SS08006_CONNECTION_FAILURE	
"Association to the dbms has failed.  This session should be disconnected."

/*
** Explanation:
**	There has been a failure in the communication between the current
**	session and the dbms.  This is because of a network failure or
**	a dbms server failure.  If you are running multiple sessions
**	this failure will not necessarily affect other sessions.
**
** System Status
**	The current query or command fails.  Any subsequent statements
**	will also fail.
**
** User Action:
**	The application should disconnect the current session.  It may
**	then attempt to reconnect or switch to a different session.
*/

E_LQ002E_NODB:SS08003_NO_CONNECTION	
"The '%0c' query has been issued outside of a DBMS session."

/*
** Explanation:
**	The specified query was issued without any connection to a DBMS.
**	This error may occur if you either did not issue a CONNECT or
**	INGRES statement, or if the connection failed and the application
**	continued.  If you are running an Embedded SQL application, make sure
**	you handle all errors. Errors from previous statements may help
**	solve the source of this error.
**
** System Status:
**	The program will continue, but errors will be raised if further
**	queries are attempted without a connection to the database.
**
** User Action:
**	The application should connect to the database before issuing
**	queries.
*/

E_LQ0030_UTBAD:SS50000_MISC_ERRORS	
"Run-time CALL statement to subsystem '%0c' failed."

/*
** Explanation:
**	The CALL statement failed.  The called subsystem is either
**	non-callable, or when it started up it issued an error of
**	its own specifying why it failed to start.
**
** System Status:
**
** User Action:
*/


E_LQ0036_INTERRUPT:SS5000L_PROTOCOL_ERROR	
"Attempt to interrupt query processing failed."

/*
** Explanation:
**	The current query could not be interrupted.  Queries are interrupted
**	when an ENDSELECT/ENDRETRIEVE is issued from within a data
**	retrieval loop, or from a direct call to the run-time procedure
**	"IIbreak".
**
** System Status:
**
** User Action:
*/

E_LQ003A_RETINIT:SS5000L_PROTOCOL_ERROR	
"Cannot start up '%0c' query.\n
Unexpected initial protocol response."

/*
** Explanation:
**	This error is issued if a SELECT/RETRIEVE statement could not
**	be processed, but did not issue any other, more specific error.
**	This error indicates that the data retrieval query was not
**	successfully processed in the DBMS.
**
** System Status:
**
** User Action:
*/

E_LQ003B_RETDESC:SS5000L_PROTOCOL_ERROR	
"Cannot execute '%0c'command.\n
Unable to read query protocol result data description."

/*
** Explanation:
**	This error is issued if a SELECT/RETRIEVE statement could not
**	be processed, but did not issue another more specific error.
**	This error indicates that the initial internal response to the 
**	data retrieval query was not formatted correctly.
**
** System Status:
**
** User Action:
*/

E_LQ003C_RETCOLS:SS21000_CARD_VIOLATION	
"Mismatching number of result columns and program variables.\n
There are %0c result columns and %1c program variables."

/*
** Explanation:
**	When using an EQUEL PARAM RETRIEVE statement, the number of result
**	columns specified in the PARAM target list (as "%format") must 
**	match the actual number of result columns that will return
**	from the query.  You cannot specify a column that is not
**	included in the format list.  For example, the following is wrong:
**
**		"%i4 = e.id, error = e.sal, %c = e.name"
**
**	The same requirement holds for SQL SELECT ... INTO statements and
**	EXECUTE PROCEDURE statements with RESULT ROW clauses.
**
** System Status:
**
** User Action:
*/


E_LQ0040_SHORTREAD:SS5000L_PROTOCOL_ERROR	
"Protocol read-failure on internal data.\n
Execution of this query is stopped."

/*
** Explanation:
**	When retrieving data from the DBMS, the data read did not satisfy
**	the read request.  If there was no other more specific
**	communications-failure error message, then try to run the current
**	query in the Terminal Monitor to confirm that the error is not
**	data-dependent.
**
** System Status:
**
** User Action:
*/

/* E_LQ0041 - E_LQ0044 reserved for CONNECTION NAME */
E_LQ0041_DUPCONNAME:SS08002_CONNECT_NAME_IN_USE
"Attempt to connect failed.  Duplicate connection name %0c."
/*
** Explanation:
**	The connection name used with the CONNECT statement is the same
**	as the connection name of an already connected database session.
**	database connections must be uniquely identified by the connection 
**	name.  The current CONNECT statement is ignored.
**
** System Status:
**
** User Action:
*/

E_LQ0042_CONNAMENOTFOUND:SS08003_NO_CONNECTION	
"Attempt to switch to a non-existent connection name."
/*
** Explanation:
**	The connection name used with the SET CONNECTION statement 
**	refers to a database session that does not currently exist.  
**
** System Status:
**
** User Action:
*/

/* E_LQ0045 - E_LQ0049 reserved for GET DIAGNOSTIC */ 

E_LQ0045_DA_INV_ITEM_TYPE:SS5000L_PROTOCOL_ERROR 
"Invalid Information Item type passed to Diagnostic Area management routine.\n  
The hex representation of the invalid Item Type is %0c."

/*
** Explanation:
**	The routine that maintains information in the SQL Diagnostic Area
**	was passed an invalid information type.  This is considered a 
**	protocol failure and is likely due either to an inconsistent
**	set of runtime libraries or to an internal coding error.
**
** System Status:
**	The application will continue, however, this item of diagnostic 
**	information will not be maintained in the Diagnostic Area.
**
** User Action:
**	Contact your System Administrator to make sure that your
**	application is running against a consistent set of INGRES run-
**	time libraries and, if necessary, relink the application.  If
**	the protocol still fails, contact your INGRES technical representative.
*/

/* E_LQ0045 - E_LQ0049 reserved for GET DIAGNOSTIC */ 

E_LQ004A_REPDEF:SS5000L_PROTOCOL_ERROR	"Error in reading or storing internal repeated query identifier."

/*
** Explanation:
**	When issuing a repeated query the run-time system stores the
**	query identifier for subsequent execution.  This error implies 
**	that either the query identifier could not be added to the internal
**	list because of memory resources, or that the DBMS was not able
**	to process the query and return the query identifier.  Convert the
**	query to a regular non-repeated query.
**
** System Status:
**
** User Action:
*/

E_LQ004B_UNDEFREP:SS5000L_PROTOCOL_ERROR	
"Repeated query '%0c' is not defined.\n
Unexpected protocol response when reading query identifier."

/*
** Explanation:
**	When issuing a repeated query the run-time system stores the
**	query identifier for subsequent execution.  This error implies 
**	the DBMS was not able to process the query and return the query
**	identifier, but returned no specific error for this failure.
**	Convert the query to a regular non-repeated query.
**	
**
** System Status:
**
** User Action:
*/

E_LQ0055_CSOPEN:SS34000_INV_CURS_NAME	
"Cursor '%0c' cannot be opened.\n
Unexpected initial protocol response."

/*
** Explanation:
**	This error is issued if a cursor OPEN statement could not
**	be processed, but did not issue any other, more specific error.
**	This error indicates that the request to open the named cursor
**	was not successfully processed in the DBMS.
**
** System Status:
**
** User Action:
*/

E_LQ0056_CSDESC:SS5000L_PROTOCOL_ERROR  
"Cursor '%0c' cannot be opened.\n
Unable to read query protocol cursor row description."

/*
** Explanation:
**	This error is issued if a cursor OPEN statement could not
**	be processed, but did not issue another more specific error.
**	This error indicates that the initial internal response to the 
**	OPEN request was not formated correctly.
**
** System Status:
**
** User Action:
*/

E_LQ0057_CSALLOC:SS50003_EXHAUSTED_RESOURCE  
"Cursor '%0c' cannot be opened.\n
Unable to allocate memory for internal cursor state."

/*
** Explanation:
**	When opening a cursor an internal descriptor is allocated to
**	maintain the cursor state.  The internal descriptor could not be
**	allocated due to the shortage of memory.  If you have many
**	cursors open at the same time, try to close some so that there
**	will be less memory demands.
**
** System Status:
**
** User Action:
*/

E_LQ0058_CSNOTOPEN:SS24000_INV_CURS_STATE	
"Cursor '%0c' not open for '%1c' command."

/*
** Explanation:
**	In order to execute an cursor statement (other than OPEN) the
**	cursor must have been successfully opened.
**
** System Status:
**
** User Action:
*/

E_LQ0059_CSINIT:SS5000L_PROTOCOL_ERROR	
"Unable to start up '%0c %1c' command.\n
Unexpected initial protocol response."

/*
** Explanation:
**	This error is issued if the specified cursor statement could not
**	be processed, but did not issue any other, more specific error.
**	This error indicates that the request to execute the statement
**	on the cursor was not successfully processed in the DBMS.
**
** System Status:
**
** User Action:
*/

E_LQ005A_CSCOLS:SS21000_CARD_VIOLATION	
"Too few columns accessed with cursor '%0c'."

/*
** Explanation:
**	A cursor retrieval must access all the columns specified in the
**	cursor declaration.  The remaining unread result data will be
**	discarded.
**
** System Status:
**
** User Action:
*/

E_LQ005B_CSXCOLS:SS21000_CARD_VIOLATION	
"Trying to access more columns than declared with '%0c %1c' command."

/*
** Explanation:
**	A cursor retrieval must only access the columns specified in the
**	cursor declaration.  The remaining result variables will be left
**	unchanged.
**
** System Status:
**
** User Action:
*/

E_LQ005C_CSRDO:SS22023_INV_PARAM_VAL	
"Incorrectly specified FOR READONLY clause in repeated cursor statement."

/*
** Explanation:
**	The FOR READONLY clause was specified in a host string variable.  The
**	variable must either be empty or must specify the FOR READONLY
**	clause (case is ignored).
**
** System Status:
**
** User Action:
*/

E_LQ005D_CSR_PFLOAD:SS5000L_PROTOCOL_ERROR	
"Data loading error encountered while pre-fetching %0c
rows for cursor '%1c'."

/*%
** Parameters:
**	%0c	Number of rows that the cursor is pre-fetching.
**	%1c	Name of cursor whose data is being retrieved.
**
** Explanation:
**	During a pre-fetch operation (for example, when fetching multiple rows
**	of data from a read-only cursor), the run-time system collects unread
**	rows from the DBMS and saves them away for subsequent retrievals.
**	This operation encountered an error whose text can be retrieved (or
**	displayed) together with this error message.
**
** System Status:
**	The run-time system aborts the current operation.  The "current" row
**	will have been retrieved, but a number of "following" pre-fetched rows
**	are discarded.
**
** User Action:
**	If this is repeatedly encountered then try setting PREFETCHROWS to 1
**	(using SET_INGRES or II_EMBED_SET) which will disable the pre-fetch
**	operation.
*/

E_LQ006A_SQMULTSEL:SS21000_CARD_VIOLATION	"The result of a singleton SELECT query or EXECUTE
PROCEDURE with RESULT ROW clause contains more than one row."

/*
** Explanation:
**	Either an SQL singleton SELECT statement or a EXECUTE PROCEDURE 
**	statement with RESULT ROW clause but no BEGIN/END block returned 
**	more than one row.  The	result variables will be assigned the data 
**	from the first row. The remaining rows will be discarded.
**
** System Status:
**
** User Action:
*/


E_LQ006B_SQNULLDYN:SS07000_DSQL_ERROR	
"The SQLDA field SQLIND for column %0c should be a non-null pointer."

/*
** Explanation:
**	A Dynamic SQL descriptor was used to EXECUTE a prepared statement
**	or FETCH from a dynamic cursor.  The data type of the specified 
**	SQLVAR was negative, though no null indicator was provided.
**
** System Status:
**
** User Action:
*/

E_LQ006C_SQVARDYN:SS07000_DSQL_ERROR	
"The SQLDA field SQLDATA for column %0c contains a null data pointer."

/*
** Explanation:
**	A Dynamic SQL descriptor was used to EXECUTE a prepared statement
**	or FETCH from a dynamic cursor.  The data address of the specified 
**	SQLVAR was zero.  The current column will be skipped during
**	processing.
**
** System Status:
**
** User Action:
*/

E_LQ006D_SQDESC:SS07000_DSQL_ERROR	
"Failed to DESCRIBE dynamic SQL statement.\n
Unable to read query protocol statement description."

/*
** Explanation:
**	This error is issued if a DESCRIBE statement could not
**	be processed, but did not issue another more specific error.
**	This error indicates that the initial internal response to the 
**	DESCRIBE request was not formated correctly.
**
** System Status:
**
** User Action:
*/

E_LQ006E_SQLDA:SS07007_NEED_USING_FOR_RES	
"The address of the SQLDA is zero."

/*
** Explanation:
**	When using the DESCRIBE statement, or processing the execution
**	of a Dynamic SQL statement with an SQLDA, the address of the
**	descriptor is zero.  If you are dynamically allocating the
**	descriptor then confirm that you have checked the return values of 
**	allocation routines, and have initialized the relevant fields of
**	the SQLDA.
**
** System Status:
**
** User Action:
*/

E_LQ006F_DYNSEL:SS07000_DSQL_ERROR 
"The INTO/USING form of the EXECUTE IMMEDIATE statement may be used only to specify a SELECT statement."

/*
** Explanation:
**	If the EXECUTE IMMEDIATE statement uses the 'INTO <variable_list>'
**	or 'USING [DESCRIPTOR] <descriptor_name>' clause, the query must
**	be a SELECT.  Non-SELECT statements do not return data and
**	do not take either of these clauses.  Verify that you initialized
**	your query buffer with a SELECT statement.
**
** System Status:
**
** User Action:
*/

E_LQ0080_QRYID:SS5000L_PROTOCOL_ERROR	
"Error in reading query or cursor internal identifier."

/*
** Explanation:
**	When opening a cursor, initially executing a repeated query,
**	or executing a database procedure, an internal query identifier
**	is returned from the DBMS.  This error indicates that the
**	identifier did not arrive to the application.
**
** System Status:
**
** User Action:
*/

E_LQ008A_RDNODESC:SS5000L_PROTOCOL_ERROR	
"No internal row descriptor for query data description."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if a request was made to read a row
**	descriptor without allocating a result area for storage.
**
** System Status:
**
** User Action:
*/

E_LQ008B_RDTUPSIZE:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in row (tuple) size."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "row
**	size" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ008C_RDCOLS:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in number of result data columns."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "attribute
**	number" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ008D_RDALLOC:SS50003_EXHAUSTED_RESOURCE	
"Failed to complete reading query data description.\n
Memory allocation for internal data structure failed."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if there was a memory resource shortage when
**	allocating internal subfields of the row descriptor.
**
** System Status:
**
** User Action:
*/

E_LQ008E_RDTYPE:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in type of result data column %0c."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "data
**	type" field of the numbered column failed.
**
** System Status:
**
** User Action:
*/

E_LQ008F_RDCOLEN:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in length of result data column %0c."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "name
**	length" field of the numbered column failed.
**
** System Status:
**
** User Action:
*/

E_LQ0090_RDCONAME:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in name of result data column %0c."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "column
**	name" field of the numbered column failed.
**
** System Status:
**
** User Action:
*/

E_LQ0091_RDFLAGS:SS5000L_PROTOCOL_ERROR	
"Failed to read query protocol data description.\n
Error in internal flags field."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal
**	"modifier" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ0092_RDTOOBIG:SS21000_CARD_VIOLATION	
"Failed to read query protocol data description.\n
Too many result columns (%0c) in description."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read too many columns
**	descriptors was encountered.
**
** System Status:
**
** User Action:
*/

E_LQ0093_RDTID:SS5000L_PROTOCOL_ERROR		
"Failed to read query protocol data description.\n
Error in internal row descriptor identifier."

/*
** Explanation:
**	Various statements require a DBMS row descriptor to be sent to
**	the application run-time system for processing.  For example, a
**	data retrieval or a COPY statement require a row descriptor.
**	This error is returned if an attempt to read the internal "row
**	network identifier" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ00A0_ATTNOINQ:SS22023_INV_PARAM_VAL	
"Attribute '%0c' cannot be inquired about."

/*
** Explanation:
**	The specified attribute cannot be inquired about in the 
**	INQUIRE_INGRES statement.
**
** System Status:
**
** User Action:
*/

E_LQ00A1_ATTNOSET:SS22023_INV_PARAM_VAL	
"Attribute '%0c' cannot be set."

/*
** Explanation:
**	The specified attribute cannot be set in the SET_INGRES statement.
**
** System Status:
**
** User Action:
*/


E_LQ00A2_ATTBADTYPE:SS22023_INV_PARAM_VAL	
"Program data can not be used for attribute '%0c'.\n
Data types are incompatible."

/*
** Explanation:
**	The data type of the specified attribute is not compatible with
**	the data type of the application data for the INQUIRE_INGRES or
**	SET_INGRES statements.
**
** System Status:
**
** User Action:
*/

E_LQ00A3_ATTSETDATA:SS22023_INV_PARAM_VAL	
"Attribute '%0c' cannot be set with value '%1c'."

/*
** Explanation:
**	An illegal value has been supplied to the SET_INGRES statement to
**	set the specified attribute.
**
** System Status:
**
** User Action:
*/

E_LQ00A4_ATTVALTYPE:SS22500_INVALID_DATA_TYPE     
"Program data can not be used for specified attribute.\n Data types are incompatible."

/*
** Explanation:
**      The data type of the specified attribute is not compatible with
**      the data type of the application data for the INQUIRE_INGRES or
**      SET_INGRES statements.  
**
** System Status:
**	Unchanged.
**
** User Action:
**	Check INQUIRE_INGRES parameters.
*/

E_LQ00A5_TRACEFILE:SS50000_MISC_ERRORS       
"Failed to open file '%0c'.\n
File '%1c' is already open for internal query trace information."

/*
** Explanation:
**	The specified file could not be opened to record internal trace 
**	information.  An attempt has been made to reset the value of "qryfile"
**	"gcafile" or "tracfile" via SET_SQL after a file has already been
**	opened and tracing has already begun.
**
** System Status:
**	Specified file will not be opened.
**
** User Action:
**	Check the logical II_EMBED_SET. Setting "qryfile", "gcafile" or 
**	"tracefile" via II_EMBED_SET will override the value specified by 
**	SET_SQL. When "printqry", "printgca" or "printtrace" are set via the
**	II_EMBED_SET logical, then SET_SQL must specify the file name prior
**	to connection to the database or the default files "iiprtqry.log"
**	"iiprtgca.log" or "iiprttrc.log" will be used. 
**	
*/

E_LQ00AD_PROCTYPE:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in type of result data column %0c."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "data
**      type" field of the BYREF parameter failed.
**
** System Status:
**
** User Action:
*/

E_LQ00AE_PROCCOLEN:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in length of result data column %0c."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "name
**      length" field of the BYREF parameter failed.
**
** System Status:
**
** User Action:
*/

E_LQ00AF_PROCCONAME:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in name of result data column %0c."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "column
**      name" field of the BYREF parameter failed.
**
** System Status:
**
** User Action:
*/

E_LQ00B0_PROCNAME:SS42506_INVALID_IDENTIFIER	
"Database procedure name is null or empty."

/*
** Explanation:
**	The database procedure name passed in for CREATE, EXECUTE or DROP
**	PROCEDURE is not a valid string.  If the procedure name was specified
**	in a host string variable, then confirm that the variable is assigned
**	a valid procedure name.  If the name was specified as a literal then
**	repreprocess the source file which accesses the procedure.
**
** System Status:
**
** User Action:
*/

E_LQ00B1_PARAMNAME:SS42506_INVALID_IDENTIFIER	
"Database procedure parameter name is invalid.\n
The name is either null, empty or has an invalid Dynamic SQL length."

/*
** Explanation:
**	The database procedure parameter name passed in with EXECUTE 
**	PROCEDURE is not a valid string.  If the parameter name was specified
**	in a host string variable, then confirm that the variable is assigned
**	a valid parameter name.  If the name was specified as a literal then
**	repreprocess the source file which executes the procedure.  If the 
**	name was assigned to the Dynamic SQL SQLNAME field verify that the
**	length is correct.
**
** System Status:
**
** User Action:
*/

E_LQ00B2_PROCDESC:SS5000L_PROTOCOL_ERROR
"Cannot execute '%0c'command.\n
Unable to read database procedure protocol result data description."

/*
** Explanation:
**	This error is issued if an EXECUTE PROCEDURE statement could not
**	be processed, but did not issue another more specific error.
**	This error indicates that the initial internal response to the 
**	parameters passed BYREF were not formatted correctly.
**
** System Status:
**	This error indicates a protocol failure.  This state of the 
**	transaction is unknown.
**
** User Action:
**	The transaction should be rolled back and tried again.
*/

E_LQ00B3_PROCNODESC:SS5000L_PROTOCOL_ERROR
"No internal row descriptor for Database Procedure BYREF parameters."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if a request was made to read a BYREF
**      parameter descriptor without allocating a result area for storage.
**
** System Status:
**
** User Action:
*/

E_LQ00B4_PROCTUPSIZE:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in row (tuple) size."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "parameter
**      list size (row size)" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ00B5_PROCFLAGS:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in internal flags field."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal
**      "modifier" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ00B6_PROCTID:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in internal row descriptor identifier."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "parameter
**      network identifier" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ00B7_PROCCOLS:SS5000L_PROTOCOL_ERROR
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Error in number of result data columns."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read the internal "attribute
**      number" field failed.
**
** System Status:
**
** User Action:
*/

E_LQ00B8_PROCTOOBIG:SS21000_CARD_VIOLATION
"Failed to read Database Procedure BYREF parameter protocol data description.\n
Too many result columns (%0c) in description."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if an attempt to read too many parameter
**      descriptors was encountered.
**
** System Status:
**
** User Action:
*/

E_LQ00B9_PROCALLOC:SS50003_EXHAUSTED_RESOURCE
"Failed to complete reading Database Procedure BYREF parameter protocol data \n
description. Memory allocation for internal data structure failed."

/*
** Explanation:
**      Various statements require a DBMS row descriptor to be sent to
**      the application run-time system for processing.  For example, a
**      data retrieval or a COPY statement require a row descriptor.
**      This error is returned if there was a memory resource shortage when
**      allocating internal subfields of the BYREF parameter (row) descriptor.
**
** System Status:
**
** User Action:
*/

E_LQ00BA_SESSDUP:SS08001_CANT_GET_CONNECTION	
"Attempt to connect to existing database session %0c."

/*
** Explanation:
**	The session identifier used with the CONNECT statement is the same
**	as the session identifier of an already connected database session.
**	Multiple database connections must be uniquely identified by the
**	user-specified session identifier.  The current CONNECT statement is
**	ignored.
**
** System Status:
**
** User Action:
*/

E_LQ00BB_SESSALLOC:SS50003_EXHAUSTED_RESOURCE	
"Unable to allocate memory for database session %0c."

/*
** Explanation:
**	When connecting to the database, an internal descriptor is
**	allocated to maintain the session's state.  The internal descriptor
**	could not be allocated due to the shortage of memory.  If you have
**	many database connections in one program or many cursors open at
**	the same time, try to close some so that there will be smaller
**	memory demands.
**
** System Status:
**
** User Action:
*/

E_LQ00BC_SESSZERO:SS08001_CANT_GET_CONNECTION
"Attempt to connect/switch/disconnect to/from session 0.  All session numbers must be positive integers; session 0 indicates a non-connected state."

/*
**
** Explanation:
**	An attempt was made to connect to,switch to, or disconnect from session
**	number 0.  Use of session 0 is not supported, as it indicates a 
**	non-connected state.
**
** System Status:
**	The attempt will fail.  The application may be left in a non-connected
**	state.
**
** User Action:
*/

E_LQ00BD_SESSNONCUR:SS08000_CONNECTION_EXCEPTION	
"Attempt to disconnect from non-current database session."

/*
** Explanation:
**	When using the an optional session identifier with the DISCONNECT
**	statement, the session specified must be the current database
**	session.  This means that the specified session must either be
**	the one that was most recently switched to by the SET_INGRES
**	statement or it must be the most recently connected session
**	with no intervening switching or disconnecting of sessions.
**	The current DISCONNECT statement is ignored.
**
** System Status:
**
** User Action:
*/

E_LQ00BE_SESSNOTFOUND:SS08003_NO_CONNECTION	
"Attempt to switch to a non-existent session."

/*
** Explanation:
**	The SESSION identifier used with the SET_INGRES statement refers
**	to a database session that does not currently exist.  The session
**	cannot be switched to.
**
** System Status:
**
** User Action:
*/

E_LQ00BF_SESSNOTFOUND:SS08003_NO_CONNECTION	
"Attempt to disconnect from a non-existent session."

/*
** Explanation:
**	The SESSION identifier or connection_name used with the DISCONNECT 
**	statement refers to a database session that does not currently exist.
**
** System Status:
**
** User Action:
*/

E_LQ00C0_SESSMIXED:SS08001_CANT_GET_CONNECTION	
"You may not have both multiple-style and single-style
CONNECT statements in one program."
/*
** Explanation:
**	The INGRES run-time system allows mutliple connections in an
**	application only if all CONNECT statements use a positive
**	session id.  This error indicates that one of the following
**	situations has occurred:  1) The current CONNECT statement has no
**	session id and previous CONNECT statements used a session id; or
**	2) The current CONNECT statement has a session id and the previous
**	CONNECT statement did not use a session id.
**
** System Status:
**	This is a fatal error.  The INGRES run-time system forces the
**	application to halt.
**
** User Action:
**	Report this error to the application developer.  All CONNECT
**	statements in a multiply-connected program must have a positive
**	session id.
*/

E_LQ00C1_LOOP_ALLOC:SS50003_EXHAUSTED_RESOURCE	
"Unable to allocate memory for loop structured
statement."
/*
** Explanation:
**	When multiple looping statements are started across multiple
**	database sessions, memory is allocated at the commencement of
**	each loop for use by the INGRES run-time library.  This memory
**	allocation has failed probably because of a shortage of memory.
**
** System Status:
**	The current loop-structured statement will be terminated.
**
** User Action:
**	If you have many database connections in one program or many
**	cursors open at the same time, try to close some so that there
**	will be smaller memory demands.
*/

E_LQ00C2_SESS_LOOP:SS25000_INV_XACT_STATE	
"Error in application control flow or in session
switching.  Attempt to execute a loop-structured statement started in session
%0d but current session is %1d."
/*
** Explanation:
**	Across multiple sessions, it is possible to start up multiple
**	loop-structured statements such as SELECT loops or database
**	procedure execution loops.  In the present situation, program
**	flow has returned from one loop to another, but the 
**	corresponding SET_INGRES SESSION statement has not been executed.
**	Alternatively, a SET_INGRES SESSION statement has been issued
**	during execution of a loop-structured statement, but program
**	control has not been transferred.  Consequently, the program
**	is attempting to execute a loop that "belongs" to another session.
**
** System Status:
**	Execution of the current block-structured statement stops.
**
** User Action:
**	Examine the interaction of session switching and statement
**	execution in the program.  Pay close attention to session
**	switching within loop-structured statements such as
**	EXECUTE PROCEDURE and the bulk SELECT.
*/

E_LQ00C5_EVALLOC:SS50003_EXHAUSTED_RESOURCE	
"Unable to allocate memory for event information."

/*
** Explanation:
**	When an event arrives from the server, the Ingres run-time
**	routines attempt to allocate memory to hold the information associated
**	with the event.  This memory allocation is unsuccessful probably
**	because of a shortage of memory.
**
** System Status:
**	Information on the event received from the dbms will not be
**	saved.  Execution of the program will continue.
**
** User Action:
**	If you have many database connections in one program or many
**	cursors open at the same time, try reducing the number of sessions
**	or cursors so that there will be fewer memory demands.
*/

E_LQ00C6_EVPROTOCOL:SS5000L_PROTOCOL_ERROR	
"Failure in server/client protocol when reading event."

/*
** Explanation:
**	The Ingres run-time routines expect event information from the
**	server to follow a predetermined protocol.  In the current
**	situation that protocol has failed and the event information and
**	attributes cannot be read.
**
** System Status:
**	The current query will most likely fail, but execution of the
**	program will continue.
**
** User Action:
**	Contact your System Administrator to make sure that your
**	application is running against a consistent set of INGRES run-
**	time libraries and, if necessary, relink the application.  If
**	the protocol still fails, contact your INGRES support person.
*/

E_LQ00C7_EVWAIT:SS25000_INV_XACT_STATE	
"The GET DBEVENT statement with the WAIT option may not be nested in another query."
/*
** Explanation:
**	When called from within a SELECT or RETRIEVE loop or from
**	a user DBEVENTHANDLER, the GET DBEVENT statement may not specify
**	the WAIT option.  This is because the INGRES run-time system
**	is currently processing query results and may not be
**	interrupted.
**
** System Status:
**	The GET DBEVENT statement will fail but program execution will
**	continue.
v**
** User Action:
**	Remove the WAIT clause from GET DBEVENT statements appearing
**	in the context of a user DBEVENTHANDLER or other handler or
**	nested within a SELECT loop.  Repreprocess, recompile and
**	relink the application.
*/

E_LQ00C8_TLS_ALLOC:SS50003_EXHAUSTED_RESOURCE	
"Unable to allocate memory for thread-local-storage."
/*
** Explanation:
**	During initialization, memory is allocated for thread-local-
**	storage of thread specific data (even on single threaded
**	systems).  This memory allocation has failed probably because 
**	of a shortage of memory.
**
** System Status:
**	The program will exit.
**
** User Action:
**	Reduce application memory requirements.
*/

E_LQ00C9_SESS_ACTIVE:SS08002_CONNECT_NAME_IN_USE
"Attempt to switch to a session which is currently active."
/*
** Explanation:
**	A session switch request (by connection name or session ID)
**	requested a session which was already active, most likely in
**	a different thread context.
**
** System Status:
**	Original session remains active.
**
** User Action:
**	Each thread must release its active session either by 
**	disconnecting, connecting a new session, or switching 
**	to an inactive session.  The session switching state-
**	ments may also be used to release an active session 
**	without activating any other session.
*/

E_LQ00CA_SESS_ACTIVE:SS08002_CONNECT_NAME_IN_USE
"Attempt to disconnect a session which is currently active."
/*
** Explanation:
**	A session disconnect request specified a session which 
**	was active in a different thread context.
**
** System Status:
**	Sessions remain connected.
**
** User Action:
**	Each thread must release its active session either by 
**	disconnecting, connecting a new session, or switching 
**	to an inactive session.  The session switching state-
**	ments may also be used to release an active session 
**	without activating any other session.
*/

E_LQ00CB_SESS_ACTIVE:SS08002_CONNECT_NAME_IN_USE
"Attempt to disconnect a session which is currently active."
/*
** Explanation:
**	A session disconnect request specified a session which 
**	was active in a different thread context.
**
** System Status:
**	Some sessions disconnected, active sessions remain connected.
**
** User Action:
**	Each thread must release its active session either by 
**	disconnecting, connecting a new session, or switching 
**	to an inactive session.  The session switching state-
**	ments may also be used to release an active session 
**	without activating any other session.
*/


/* 
**  Ingres65 XA Support - Error Messages will run from 0D0 to 0DF.
**
*/

E_LQ00D0_XA_INTERNAL:SS50000_MISC_ERRORS
"Internal Error in the XA component. Contact Ingres Customer \n
Support for further assistance. The error is: %0c."


/*
** Explanation:
**      An internal error was found in the optional Ingres' LIBQXA component
**      that links into the application program. This is unexpected. Please
**      contact Ingres CS&S folks for follow-up and assistance.
**
** System Status:
**
** User Action:
*/


E_LQ00D1_XA_USER_OR_TM_ERR:SS22023_INV_PARAM_VAL
"Error at the TM-RM's XA interface. Check the AS initialization sequence, \n
as well as the TP system or Ingres documentation. The error code \n
is: %0c."


/*
** Explanation:
**      An internal error was found in the optional Ingres' LIBQXA component
**      that links into the application program. This is unexpected. Please
**      contact Ingres CS&S folks for follow-up and assistance.
**
** System Status:
**
** User Action:
*/

E_LQ00D2_XA_XN_ROLLBACKONLY:SS40000_XACT_ROLLBACK
"Your transaction is being rolledback/was rolled back alread by the resource
manager.  The ROLLBACK value is: %0c.  Other error messages may have preceded
this one indicating why the transaction was rolled back."

/*
** Explanation:
**	Either the user tried to issue a transaction demarcation statement
**	directly, or the server was forced to rollback because of an error
**	(like deadlock).
**
** System Status:
**
** User Action:
*/

E_LQ00D3_XA_ILLEGAL_XACT_STMT:SS2D000_INV_XACT_TERMINATION
"an attempt was made to terminate the current SQL-transaction by means of a commit or rollback statement inside the application, which is dis-allowed by the XA protocol now in effect.  This transaction (branch) will be rolled back.  Note that a commit or rollback statement executed from a database procedure can also cause this error."

/*
** Explanation:
**      - an attempt is made to terminate the current SQL-transaction by means
**        of <commit statement> and the current SQL-transaction is a part of an
**        encompassing transaction that is controlled by an agent other than the
**        SQL-agent or
**      - an attempt is made to terminate the current SQL-transaction by means
**        of explicitly executed <rollback statement> and the current
**        SQL-transaction is a part of an encompassing transaction that is
**        controlled by an agent other than the SQL-agent
**
** System Status:
**	The transaction will be rolled back.
**
** User Action:
**	Remove the offending statement from the application or database
**	procedure.
*/

E_LQ00D4_XA_BAD_VALUE:SS50000_MISC_ERRORS
"Unable to process the value specified in your %0c environment variable."

/*
** Explanation:
**	The a routine used to process your environment variable returned an
**	error.  Please check the value and confirm it is correct.
**
** System Status:
**	The system will behave as if the variable was not set at all.  This 
**	may mean you get a default value, or a feature may not be enabled/
**	disabled.
**
** User Action:
**	Ensure the value in the variable is correct and try the application
**	again.
*/

E_LQ00D5_XA_ILLEGAL_STMT:SS50008_UNSUPPORTED_STMT
"The last SQL statement is illegal in the context of an XA Application Server.
The transaction branch will be marked ROLLBACK ONLY"

/*
** Explanation:
**	Certain SQL statements are disallowed in the context of an XA 
**	application.  The INGRES runtime system has detected the execution
**	of such a statement.  Transaction integrity can not be verified,
**	so the transaction branch will be rolled back.
**
** System Status:
**	The transaction will be rolled back.
**
** User Action:
**	Remove the offending statement from the application or database
**	procedure.
*/

E_LQ00D6_XA_OUTSIDE_STMT:SS40000_XACT_ROLLBACK
"An SQL statement was executed outside the context of an XA transaction (i.e.,
no valid transaction branch association exists).  The transaction branch 
associated with the current SQL session will be marked ROLLBACK ONLY"

/*
** Explanation:
**	An SQL statement was executed in a session which is not legally
**	associated to a thread of control.
**
** System Status:
**	If the session has an active transaction, that transaction will be
**	rolled back.
**
** User Action:
**	This is most likely caused by a logic error.  It is suggested the 
**	developer make use of existing tracing facilities to diagnose the 
**	problem.  See II_XA_TRACE_FILE in the XA reference manual.
*/

E_LQ00D7_XA_AMBIG_CONNECTION:SS08003_NO_CONNECTION
"The last SQL statement was executed in the context of an XA transaction which involves more than one resource manager instance (RMI), but the target RMI was not specified via a SET CONNECTION statement.  The transaction branch will be marked ROLLBACK ONLY"

/*
** Explanation:
**	When more than one Resource Manager Instance (RMI) is involved in 
**	a transaction, the target RMI must be set with a SET CONNECTION 
**	statement before any SQL statements can be executed.  The last
**	SQL statement was executed before such a SET CONNECTION was issued.
**
** System Status:
**	The transaction will be rolled back.
**
** User Action:
**	Insert SET CONNECTION into the application at the start of any
**	service routine that involves move than one RMI.
*/

E_LQ00D8_XA_CB_ALLOC:SS50003_EXHAUSTED_RESOURCE
 "Unable to allocate memory: cl_status = %0c \n
for control block = %1c."

/*
** Explanation:
**	When connecting to the database, an internal descriptor is
**	allocated to maintain the database session's state. If this is an
**      XA application, each active XA transaction at this application server
**      (AS) also requires a number of internal structures. Either of these
**      could not be allocated due to the shortage of memory. 
**
** System Status:
**      The current transaction will likely be rolled back.
**
** User Action:
** Try one of the following:
**      a) reduce the number of database connections and/or concurrent XA
**         transactions in this application program, (start more ASs)
**      b) try closing some cursors, if too many are open at the same time.
*/

E_LQ00D9_XA_TUXEDO_ERROR:SS50000_MISC_ERRORS
 "Unexpected error when calling a TUXEDO service.  The following text summarizes the error.  Please see the relevant log file for more info: %0c"

/*
** Explanation:
**	INGRES/OpenDTP for TUXEDO reliese on TUXEDO to perform certain internal
**	functions.  A call to a TUXEDO routine failed unexpectedly.
**
** System Status:
**	The current XA transaction will likely be rolled back.
**
** User Action:
**	If XA tracing is enabled, consult the trace logs.  Consult all other
**	relevant logs for information regarding this error.
*/

E_LQ00DA_XA_NOAUTH:SS0A000_FEATUR_NOT_SUPPORTED
"No INGRES capability for the TM-XA interface."

/*
** Explanation:
**      Based on the installed INGRES authorization key you are not allowed
**      to use this feature
**
** System Status:
**      The application will exit. 
**
** User Action:
**      Check with your INGRES system administrator for the desired TM-XA
**      interface.
**	relevant logs for information regarding this error.
*/

E_LQ00E0_NODATAHDLR:SS22023_INV_PARAM_VAL
"The function specified in the DATAHANDLER clause is NULL."
/*
** Explanation:
**      The DATAHANDLER Clause specified a NULL function.
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**     Specify a valid user-defined function in the DATAHANDLER clause of
**     of the query.  If the datahandler argument to the clause is a
**     function pointer, ensure that it is set to a valid user-defined
**     function contained in the program.
*/

E_LQ00E1_DATHDLRTYPE:SS22500_INVALID_DATA_TYPE
"Invalid column  data type for datahandler.\n 
The DATAHANDLER clause is valid for columns of 'long' datatypes only."
/*
** Explanation:
**	The column for which the DATAHANDLER clause was specified is not of
**	type 'long'.  The datahandler clause is only valid for
**	columns of type long varchar, long byte or long bit.
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Use the standard sql method of retrieving the value into a program
**	variable.
*/

E_LQ00E2_GETHDLRONLY:SS42000_SYN_OR_ACCESS_ERR
"The %0c statement may only be issued from within
a datahandler specified for retrieving data."
/*
** Explanation:
**	A statement was issued which is only valid from within a DATAHANDLER
**	specified for retrieving data.  A GET DATA or ENDDATA statement may
**	have been issued outside of a user defined datahandler, or a 
**	PUT DATA statement may have been issued from within a datahandler
**	specified for retrieving data (SELECT, FETCH ...).
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**      Ensure that that function specified in the DATAHANDLER clause is
**      a function for retrieving data from the database.  
**	Check GET DATA, PUT DATA and ENDDATA statements to ensure that they 
**	are issued within the correct datahandler type.
*/

E_LQ00E3_GETNOLEN:SS42000_SYN_OR_ACCESS_ERR
"The GET DATA statement must include the 'with maxlength' clause when the segment variable is of type pointer."
/*
** Explanation:
**      A GET DATA statement has specified a segment variable of type pointer 
**	without including the "with maxlength" clause.  It is impossible
**      for the Ingres run-time system to determine how large a segment to
**	retrieve into the segment program variable.  The Ingres run-time
**	system must assume that since a datahandler was specified, the 
**	intention of the application was not to retrieve the entire object.
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Add the "WITH MAXLENGTH" clause to the GET DATA statement.
*/

E_LQ00E4_NOENDDATA:SS50000_MISC_ERRORS
"Premature exit from datahandler.  Datahandler exited before all of the data was retrieved.  Issue the ENDDATA statement to discard remaining data."
/*
** Explanation:
**	The datahandler has returned without retrieving all of the data for
**	the column.  The entire column value must be retrieved or an ENDDATA 
**	statement must be issued to skip the remaining segments of the column.
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Add EXEC SQL ENDDATA statement to the datahandler when you do not wish 
**	to retrieve the remaining segments of the column.
**
*/

E_LQ00E5_PUTHDLRONLY:SS42000_SYN_OR_ACCESS_ERR
"The %0c statement may only be issued from within
a datahandler specified for inserting data."
/*
** Explanation:
**	A statement was issued which is only valid from within a DATAHANDLER
**	specified for inserting data into the database.  A PUT DATA statement
**	may have been issued outside of a user defined datahandler or a
**	GET DATA statement may have been issued from within a datahandler
**	specified for inserting data (INSERT, UPDATE ...).
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Ensure that that function specified in the DATAHANDLER clause is
**	a function for inserting data into the database.
**
*/

E_LQ00E6_PUTNOLEN:SS42000_SYN_OR_ACCESS_ERR
"The PUT DATA statement must include the 'segmentlength' parameter when the segment variable is of type pointer."
/*
** Explanation:
**	A PUT DATA statement has specified a SEGMENT variable of type pointer 
**	without specifying the SEGMENTLENGTH.  Since segments are not assumed 
**	to be null-terminated it is impossible for the Ingres run-time system 
**	to determine how many bytes in the program variable are valid. 
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Specify the SEGMENTLENGTH in the PUT DATA statement.
*/

E_LQ00E7_NODATAEND:SS50000_MISC_ERRORS
"No DATAEND indicator.  Datahandler exited before specifying the end of data.  To specify end of data set the DATAEND indicator to '1' in a PUT DATA statement."
/*
** Explanation:
**	A datahandler has returned without specifying the end of data. A
**	datahandler for inserting data into the database never issued a
**	PUT DATA statement with the parameter DATAEND set to "1".
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Set the DATAEND parameter in the last PUT DATA statement to "1" before
**	exiting the datahandler.
**
*/

E_LQ00E8_ADFALLOC:SS50003_EXHAUSTED_RESOURCE
"Unable to allocate memory for Abstract Data-Type Facility for database session %0c."

/*
** Explanation:
**	When connecting to the database, an ADF control block is
**	allocated for each session.  This error indicates that an
**	ADF control block could not be allocated due to a shortage of memory.
**	If you have many database connections in one program, try to close
**	some connections so that there will be smaller memory demands.
**
** System Status:
**
** User Action:
*/

E_LQ00E9_BADSEGLEN:SS50000_MISC_ERRORS
"Invalid SEGMENTLENGTH value (%0d) specified in the PUT DATA statement.  The 
value must be a legal length specificaton that does not exceed the size of the 
segment variable."
/*
** Explanation:
**      The segmentlength parameter to the PUT DATA statement contains an
**	invalid value. This error indicates that the segment length specified
**	is either negative, or longer than the host program variable which
**	was specified to inserting the data.
**
** System Status:
**	Current query will fail, but execution of program will continue.
**
** User Action:
**	Ensure that the SEGMENTLENGTH specified in the PUT DATA statement is
**      a positive integer that does not exceed the size of the program
**	variable that contains the large object segment.
*/
E_LQ00EA_PROMPT_ALLOC:SS50003_EXHAUSTED_RESOURCE	
"Unable to allocate memory for dbprompt information."
/*
** Explanation:
**	When an prompt arrives from the server, the Ingres run-time
**	routines attempt to allocate memory to hold the information associated
**	with the prompt.  This memory allocation is unsuccessful probably
**	because of a shortage of memory.
**
** System Status:
**	The prompt is rejected.
**
** User Action:
**	If you have many database connections in one program or many
**	cursors open at the same time, try reducing the number of sessions
**	or cursors so that there will be fewer memory demands.
*/

E_LQ00EB_PROMPT_PROTOCOL:SS5000L_PROTOCOL_ERROR	
"Failure in server/client protocol when reading dbprompt."

/*
** Explanation:
**	The Ingres run-time routines expect prompt information from the
**	server to follow a predetermined protocol.  In the current
**	situation that protocol has failed and the prompt information and
**	attributes cannot be read.
**
** System Status:
**	The current query will most likely fail, but execution of the
**	program will continue.
**
** User Action:
**	Contact your System Administrator to make sure that your
**	application is running against a consistent set of INGRES run-
**	time libraries and, if necessary, relink the application.  If
**	the protocol still fails, contact your INGRES support person.
*/


E_LQ00EC_UNTERM_C_STRING:SS22024_UNTERM_C_STRING	
"Unable to convert program data. \n
A host variable contains an unterminated C string."

/*
** Explanation:
**	When setting data from a host variable, an unterminated C string was
**	found, and thus, no data is assigned. ESQL/C requires the user to
**	null terminate their C strings in order to pass data correctly to
**	the server.
**
** System Status:
**
** User Action:
**	This is a user programming error, correct your code and rebuild 
*/

W_LQ00ED_ING_SET_USAGE:SS50008_UNSUPPORTED_STMT
"WARNING: Incorrect setting for ING_SET, ING_SYSTEM_SET, or \n
ING_SET_DBNAME variables. ING_SET, ING_SYSTEM_SET, or ING_SET_DBNAME \n
only support set statements. Your current statement will not be executed." 

/*
** Explanation:
**	ING_SET, ING_SYSTEM_SET, and ING_SET_DBNAME by deffinition 
**	should only support set statements.  If statement other than 
**	set statement assigned to one of these variables, it will
**	not be executed for security reasons, instead it will output 
**	a warning.  
**	
** System status:
**
** User Action:
**	Assign a set statement to ING_SET, ING_SYSTEM_SET, and 
**	ING_SET_DBNAME variables.
**
*/


/* 
** Slow messages follow
*/

/*
**  The following three messages print file and line number information
**  and are followed by the text of the current query.  One of these
**  messages and the query text gets printed only if the embedded program
**  is being run in debugging mode (ie., preprocessor used with '-d' flag.).
*/

S_LQ0200_DEBUG1:SS50000_MISC_ERRORS
"Last INGRES statement in file '%0c', line %1d:"

S_LQ0201_DEBUG2:SS50000_MISC_ERRORS
"Last INGRES statement in file '%0c':"

S_LQ0202_DEBUG3:SS50000_MISC_ERRORS
"Last INGRES statement:"

/* This message is issued when the GCA layer forces us to exit */
S_LQ0210_QUIT:SS50006_FATAL_ERROR	
"Exiting session because of communications failure.\n"

S_LQ0211_SINOTOPEN:SS5000M_IPC_ERROR
"%0c: Cannot open standard program channels."

/* Message to help user know when end of data after CALL statement */
S_LQ0212_CALL_CONTINUE:SS5000K_SQLSTATE_UNAVAILABLE
"Press RETURN to continue: "

/* 
**  Fast messages follow
*/

/* Message banner for user messages from database procedures */
F_LQ0301_UMSG_BOTH	"MESSAGE %0d: %1c"
F_LQ0302_UMSG_TEXT	"MESSAGE: %0c"
F_LQ0303_UMSG_NUMBER	"MESSAGE: %0d"

NO_SQLSTATE_MAPPING
