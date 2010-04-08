SQLSTATE_MAPPING_REQUIRED

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erlc.msg -	Message file for class LC
**
** Description:
**	This file defines the messages for class LC (LIBQ-to-Communications
**	layer errors).
**
**	Error formats:
**
**	E_LC0001   	- 09	General
**	E_LC0010_ASSOC 	- 19	Association
**	E_LC0020_READ   - 27	Read
**	E_LC0028_AREAD  - 2F	Alternate Read
**	E_LC0030_WRITE	- 39	Write
**
** History:
**	 16-sep-1987 (neil)
**		Created.
**	 08-jul-1988 (neil)
**		Added comments for all error messages.
**	20-apr-1990 (barbara)
**		Added ER_LC0014_ASSOC_ALLOC so that MEreqmem errors
**		no longer go through IIhdl_gca().
**	08-may-1990 (neil)
**		New errors for "alternate reading".
**	25-aug-92 (mgw)
**		Added 6.5 II_TIMEZONE_NAME error E_LC0037_BAD_TIMEZONE_NAME
**	01-nov-1992 (larrym)
**		Added SQLSTATEs.
**	05-Jan-2007 (bonro01)
**		Change Getting Started Guide to Installation Guide.
**/

# define	_LC_CLASS	152



E_LC0001_SERVICE_FAIL:SS5000M_IPC_ERROR
"GCA protocol service (%0c) failure.\n
Internal service status %1c."

/*
** Explanation:
**	GCA protocol services are those used to communicate with a gateway
**	or DBMS server.  Most services are associated with either sending
**	or receiving data.  The first parameter specifies the internal
**	service name.  The second parameter specifies the status that returned
**	from GCA.  This status may also include a system-dependent status
**	reason, or, if unavailable, will just be a hex number.  Under most
**	situations the front-end communication interface will terminate
**	processing.
**
** System Status:
**
** User Action:
*/

E_LC0010_ASSOC_REJECT:SS08004_CONNECTION_REJECTED
"Request to connect to server rejected (protocol message type %0c)."

/*
** Explanation:
**	In most cases connection errors are the result of the failure to
**	connect to the database (for example, the database did not exist,
**	you have no permission, etc).  In some cases, the connection error
**	is due to a communication protocol error.  In this case the above
**	error is returned.  The parameter specifies the data type of the
**	protocol message that rejected the connection request.
**
** System Status:
**
** User Action:
*/

E_LC0012_ASSOC_RESTORE_DATA:SS5000M_IPC_ERROR
"Error in internal DBMS session restoration data.\n
Missing '%0c' operator in saved data string."

/*
** Explanation:
**	When you issue a CALL statement, or you are in an INGRES front-end
**	that calls up another INGRES subsystem or ABF application, the
**	run-time system attaches both processes to the same communication
**	channel.  The second invoked process restores its communicate state
**	from data passed in by the first process.  This data was not of
**	the expected syntax and consequently the second process could not
**	be invoked.
**
** System Status:
**
** User Action:
*/

E_LC0013_ASSOC_NO_NAME:SS08001_CANT_GET_CONNECTION
"The symbol '%0c' has not been assigned a value.\n
Unable to connect to DBMS session."

/*
** Explanation:
**	In order to connect to a database certain symbols must be assigned
**	a value.  The specified symbol was not assigned a value and the
**	connection failed.  Consult the INGRES Installation Guide for more
**	details on the meaning of the specified symbol.
**
** System Status:
**
** User Action:
*/

E_LC0014_ASSOC_ALLOC:SS50003_EXHAUSTED_RESOURCE
"Attempt to allocate memory failed with code %0c.
No connection made to DBMS."

/*
** Explanation:
**	The INGRES run-time system failed in its attempt to allocate
**	the memory needed for establishing a connection with the dbms.
**	Without this memory, there can be no communication with the
**	dbms server.  Therefore the connection attempt is aborted.
**	
** System Status:
**	All database connections are automatically closed.
**
** User Action:
**	If this error is actually due to a memory shortage, it may
**	help to reduce memory demands by having fewer concurrent
**	database connections within the application.  Occasionally,
**	this error will occur due to a memory trashing bug in user code.
*/

E_LC0020_READ_SETUP_FAIL:SS5000L_PROTOCOL_ERROR
"Protocol read setup failure.\n
Expected to read message type %0c, but read type %1c with %2c bytes."

/*
** Explanation:
**	Certain queries require the application to read formatted data
**	from the DBMS.  For example, a cursor FETCH reads a row from
**	the database.  The attempt to read failed and the wrong
**	communications message type was returned.  The type returned and
**	the number of bytes will help isolate the problem.
**
** System Status:
**
** User Action:
*/

E_LC0021_READ_SHORT_DATA:SS5000L_PROTOCOL_ERROR
"Protocol data read failure.\n
At end of message %0c, expected to read %1c bytes but read only %2c bytes."

/*
** Explanation:
**	Certain queries require the application to read formatted data
**	from the DBMS.  For example, a cursor FETCH reads a row from
**	the database.  The attempt to read failed due to the early
**	termination of the communications message.  For example, the
**	cursor row was shorter than specified in the initial internal
**	descriptor.
**
** System Status:
**
** User Action:
*/

E_LC0022_READ_WRONG_MESSAGE:SS5000L_PROTOCOL_ERROR
"Protocol data read failure.\n
Found message %0c while reading message %1c, and read only %2c bytes."

/*
** Explanation:
**	Certain queries require the application to read formatted data
**	from the DBMS.  For example, a cursor FETCH reads a row from
**	the database.  The attempt to read failed due to the early
**	termination of the communications message and its continuation
**	into an unrelated communications message.  For example, an internal
**	identifier overflowed into the data portion of a cursor row.
**
** System Status:
**
** User Action:
*/

E_LC0023_READ_AFTER_EOD:SS5000L_PROTOCOL_ERROR
"Protocol data read failure.\n
Trying to read from message %0c after detecting end of data."

/*
** Explanation:
**	Certain queries require the application to read formatted data
**	from the DBMS.  For example, a cursor FETCH reads a row from
**	the database.  The attempt to read failed because all the requested
**	results of the query were already processed and there are no more.
**	For example, an internal row descriptor	has already been read
**	but an attempt is made to read more of it.
**
** System Status:
**
** User Action:
*/

E_LC0024_READ_BAD_TYPE:SS5000L_PROTOCOL_ERROR
"Protocol read failure.\n
Ignoring invalid or unknown message type %0c from GCA_INTERPRET."

/*
** Explanation:
**	All queries read something from the DBMS.  Some queries read
**	formatted data (for example a cursor row) while others just read
**	a status.  The attempt to read failed due to an undefined
**	communications message type being returned.
**
** System Status:
**
** User Action:
*/

E_LC0025_READ_ASSOC_FAIL:SS5000M_IPC_ERROR
"Protocol read failure.\n
Association with database partner failed (GCA_RECEIVE) with status %0c."

/*
** Explanation:
**	All queries read something from the DBMS.  Some queries read
**	formatted data (for example a cursor row) while others just read
**	a status.  The attempt failed due to a failure in the communications
**	system (for example, your network connection may have failed).
**	The parameter specifies the status that returned from GCA.  This
**	status may also include a system-dependent status reason, or, if
**	unavailable, will just be a hex number.  Under all situations the
**	front-end communication interface will terminate processing.
**
** System Status:
**
** User Action:
*/

E_LC0028_AREAD_LOAD_ALLOC:SS50003_EXHAUSTED_RESOURCE
"Pre-fetch buffer allocation failure.\n
An attempt to allocate a %0c message buffer with %1c bytes failed.  The
current operation is aborted."

/*%
** Parameters:
**	%0c	Internal name of message buffer (for example GCA_TUPLES).
**	%1c	Size of message buffer in bytes.
**
** Explanation:
**	During a pre-fetch operation (for example, when fetching multiple rows
**	of data from a read-only cursor), the run-time system attempts to
**	allocate unread message buffers to improve the performance of
**	subsequent retrievals against the same data.  When trying to allocate
**	one such buffer the system encountered a memory failure.  This is
**	normally due to memory resources being exhausted for the process.
**	
** System Status:
**	The run-time system aborts the current operation.  If this is a cursor
**	FETCH operation then the "current" row will have been retrieved, but
**	a number of "following" pre-fetched rows will have been discarded.
**
** User Action:
**	Confirm that you are not encountering unrelated memory errors that
**	could be due to your program corrupting the internal memory management
**	lists.  If this only occurs when trying to pre-fetch data then maybe
**	the system has exhausted all memory.  For example, a large number of
**	read-only cursors all open at the same time, all having set
**	the PREFETCHROWS value to a very large number, may encounter this
**	error.  This error can be avoided by setting PREFETCHROWS to a low
**	number (if the default is too high) or to 1 (which will disable
**	message buffering all together).
*/

E_LC0029_AREAD_ROW_COUNT:SS5000L_PROTOCOL_ERROR
"Pre-fetch requested row count exceeded.\n
While reading a %0c message, a requested pre-fetch data row count of %1c
was exceeded with a buffer row count of %2c.  All buffered data is ignored."

/*%
** Parameters:
**	%0c	Internal name of message buffer (for example GCA_TUPLES).
**	%1c	Number of rows originally requested for pre-fetch operation.
**	%2c	Number of rows encountered so far.
**
** Explanation:
**	During a pre-fetch operation (for example, when fetching multiple rows
**	of data from a read-only cursor), the run-time system collects unread
**	rows from the DBMS and saves them away for subsequent retrievals.
**	The requested number of rows (determined by the PREFETCHROWS setting or
**	the system default) has been exceeded, in violation of the
**	client-server GCA protocol.  
**	
**	A message type of GCA_TUPLES indicates that the actual data exceeded
**	the row count.  A message type of GCA_RESPONSE indicates that the
**	internal DBMS response count exceeded the value, though the data may
**	have been correct (this latter problem is less severe).
**
** System Status:
**	The run-time system aborts the current operation.  If this is a cursor
**	FETCH operation then the "current" row will have been retrieved, but
**	a number of "following" pre-fetched rows will have been discarded.
**
** User Action:
**	If this is repeatedly encountered then try setting PREFETCHROWS to 1
**	which will disable the pre-fetch operation.
*/

E_LC002A_AREAD_WRONG_MESSAGE:SS5000L_PROTOCOL_ERROR
"Protocol data pre-fetch failure.\n
During a pre-fetch operation the system expected messages of types(s) %0c,
but encountered a %1c message."

/*%
** Parameters:
**	%0c	Internal name of message buffers that are expected.
**	%1c	Internal name of message buffer encountered.
**
** Explanation:
**	During a pre-fetch operation (for example, when fetching multiple rows
**	of data from a read-only cursor), the run-time system collects unread
**	messages from the DBMS and saves them away for subsequent retrievals.
**	During this operation the system encountered an unexpected message,
**	in violation of the client-server GCA protocol.  The message could not
**	be interpreted in the current context.
**
** System Status:
**	The run-time system aborts the current operation.  If this is a cursor
**	FETCH operation then the "current" row will have been retrieved, but
**	a number of "following" pre-fetched rows will have been discarded.
**
** User Action:
**	If this is repeatedly encountered then try setting PREFETCHROWS to 1
**	which will disable the pre-fetch operation.
*/

E_LC002B_AREAD_SHORT_DATA:SS5000L_PROTOCOL_ERROR
"Read failure against pre-fetched data.\n
The system encountered the end of the pre-fetched message stream.  Expected
to read %0c bytes but read only %1c bytes from a %2c message."

/*%
** Parameters:
**	%0c	Number of bytes expected to read.
**	%1c	Number of bytes actually read when end of stream encountered.
**	%2c	Message where data was being read from.
**
** Explanation:
**	During a read operation from a pre-fetched data stream (for example,
**	when multiple rows of data from a read-only cursor have been
**	pre-fetched and more data is now being retrieved), the run-time system
**	encountered the early termination of the data stream.
**
** System Status:
**	The run-time system aborts the current operation.  If this is a cursor
**	FETCH operation then the "current" row will not have been retrieved,
**	but subsequent FETCH operations should succeed.
**
** User Action:
**	If this is repeatedly encountered then try setting PREFETCHROWS to 1
**	which will disable the pre-fetch operation.  If you encountered
**	earlier errors while trying to retrieve from the same cursor, then
**	correct those errors and try again.
*/

E_LC0030_WRITE_SEND_FAIL:SS5000M_IPC_ERROR
"GCA protocol service (%0c) failure with message type %1c.\n
Internal service status %2c."

/*
** Explanation:
**	All queries and database requests are sent via the communications
**	system, GCA. The first parameter specifies the internal GCA
**	service name.  The second parameter specifies the message type
**	that the application was trying to send (for example, FETCH or QUERY).
**	The last parameter specifies the status that returned from GCA.
**	This status may also include a system-dependent status
**	reason, or, if unavailable, will just be a hex number.  Under most
**	situations the front-end communication interface will terminate
**	processing.
**
**
** System Status:
**
** User Action:
*/

E_LC0031_WRITE_ASSOC_FAIL:SS5000M_IPC_ERROR
"Protocol write failure.\n
Association with database partner failed (GCA_SEND) with status %0c."

/*
** Explanation:
**	All queries and database requests are sent via the communications
**	system, GCA.  The attempt failed due to a failure in the communications
**	system (for example, your network connection may have failed).
**	The parameter specifies the status that returned from GCA.
**	This status may also include a system-dependent status
**	reason, or, if unavailable, will just be a hex number.  Under most
**	situations the front-end communication interface will terminate
**	processing.
**
** System Status:
**
** User Action:
*/

E_LC0035_BAD_RETRY_VALUE:SS08001_CANT_GET_CONNECTION
"Bad numeric value for II_CONNECT_RETRIES.\n 
The symbol value (%0c) isn't a reasonable retry count."  
 
/*
** Explanation:
**      The symbol II_CONNECT_RETRIES should contain a numeric value
**      indicating the number of times a connect should be retried before
**      declaring an error.  The value is optionally followed by either
**      the letter 'd' or the letter 'D', indicating that there should
**      be some delay between attempts.  By default, retries happen
**      immediately.
**
** System Status:
**
** User Action:
**      Set the symbol to a reasonable value, or unset the symbol to get
**	the default of 1 try.
*/

E_LC0036_BAD_RETRY_SYNTAX:SS08001_CANT_GET_CONNECTION
"Syntax error in II_CONNECT_RETRIES.\n
The value `%0c' isn't legal syntax for the symbol.  The
correct syntax is: number[d|D], a number optionally followed by 'd' or 'D'."
 
/*
** Explanation:
**      The symbol II_CONNECT_RETRIES should contain a numeric value
**      indicating the number of times a connect should be retried before
**      declaring an error.  The value is optionally followed by either
**      the letter 'd' or the letter 'D', indicating that there should
**      be some delay between attempts.  By default, retries happen
**      immediately.
**
** System Status:
**
** User Action:
**      Set the symbol to a reasonable value, or unset the symbol to get
**	the default of 1 try.
*/

E_LC0037_BAD_TIMEZONE_NAME:SS08001_CANT_GET_CONNECTION
"II_TIMEZONE_NAME not set.\n
You have 6.5 or greater frontends. When these were installed II_TIMEZONE_NAME
should have been set to the proper timezone. Please consult the
Installation Guide for information on the setting of II_TIMEZONE_NAME"
 
/*
** Explanation:
**      II_TIMEZONE_NAME should have the name of the user's timezone set in it.
**	Consult the Installation Guide for the proper setting.
**
** System Status:
**
** User Action:
**	See Installation Guide for proper setting of
**	II_TIMEZONE_NAME and set it properly.
*/

/*
** Slow messages follow
*/

/* 
**  Fast messages follow
*/

NO_SQLSTATE_MAPPING
