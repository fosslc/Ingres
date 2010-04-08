/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ErrRsrc.java
**
** Description:
**	Locale specific information.  This file defines the
**	default resources values (english,USA) for error
**	codes.  These resources may be localized for
**	different languages and customs by providing suitably
**	named classes (see documentation for ResourceBundle)
**	which define the same keys as this file and provides 
**	the translated values.
**
**  Classes
**
**	ErrRsrc		Extends java.util.ListResourceBundle
**	EmptyRsrc	Empty ResourceBundle.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Moved to util package.
**	13-Jun-00 (gordy)
**	    Added procedure call syntax error E_JD0016.
**	30-Oct-00 (gordy)
**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
**	31-Jan-01 (gordy)
**	    Added invalid date format E_JD0019_INVALID_DATE.
**      29-Aug-01 (loera01) SIR 105641
**          Added invalid output parameter: E_JD0020_INVALID_OUT_PARAM.
**          Added no result set returned: E_JD0021_NO_RESULT_SET.
**          Added result set not permitted: E_JD0022_RESULT_SET_NOT_PERMITTED.
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Renamed to remove specific product reference.
**	    Translated JDBC error codes into new GCF error codes.
**	26-Dec-02 (gordy)
**	    Added bad character set E_GC4009.
**	14-Feb-03 (gordy)
**	    Added invalid transaction state E_GC401F and missing
**	    parameter E_GC4020.
**	 1-Nov-03 (gordy)
**	    Added invalid row request E_GC4021.
**	03-jan-05 (devjo01)
**	    Correct spelling of "concurrancy" in E_GC4016_RS_CHANGED.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	 5-Dec-07 (gordy)
**	    Added additional server error codes to provide local exceptions.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	11-Jun-09 (rajus01) SIR 121238
**	    Added invalid LOB object E_GC4022.
**	23-Jun-09 (rajus01) SIR 121238
**	    Added invalid interface E_GC4023 for unwrap().
*/

import	java.util.ResourceBundle;
import	java.util.ListResourceBundle;

/*
** Name: ErrRsrc
**
** Description:
**	ResourceBundle sub-class which defines the default resource
**	keys and values (english,USA) for error codes.
**
**
**  Public Data:
**
**    Resource keys for error messages.
**
**	E_GC4000
**	E_GC4001
**	E_GC4002
**	E_GC4003
**	E_GC4004
**	E_GC4005
**	E_GC4006
**	E_GC4007
**	E_GC4008
**	E_GC4009
**	E_GC4010
**	E_GC4011
**	E_GC4012
**	E_GC4013
**	E_GC4014
**	E_GC4015
**	E_GC4016
**	E_GC4017
**	E_GC4018
**	E_GC4019
**	E_GC401A
**	E_GC401B
**	E_GC401C
**	E_GC401D
**	E_GC401E
**	E_GC401F
**	E_GC4020
**	E_GC4021
**
**	E_GC480D
**	E_GC480E
**	E_GC4811
**	E_GC481E
**	E_GC481F
**
**  Public methods:
**
**	getContents	Returns the array of keys/values for defined Locale.
**	getResource	Returns the resources for default Locale
**
**  Private Data:
**
**	info		Array of keys/values.
**	RSRC_CLASS	Base resource class.
**	resource	Resource for default Locale.
**
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added BLOB support: E_JD000B_BLOB_IO,
**	    E_JD000C_BLOB_ACTIVE, E_JD000D_BLOB_DONE.
**	26-Oct-99 (gordy)
**	    Added ODBC escape parsing: E_JD000E_UNMATCHED.
**	16-Nov-99 (gordy)
**	    Added query timeouts: E_JD000F_TIMEOUT.
**	11-Nov-99 (rajus01)
**	    Implement DatabaseMetaData: Added E_JD0010_CATALOG_UNSUPPORTED,
**	    E_JD0011_SQLTYPE_UNSUPPORTED.
**	21-Dec-99 (gordy)
**	    Added generic unsupported: E_JD0012_UNSUPPORTED.
**	25-Jan-00 (rajus01)
**	    Added resultsSet closed: E_JD0013_RESULTSET_CLOSED.
**	31-Jan-00 (rajus01)
**	    Added column not found: E_JD0014_INVALID_COLUMN_NAME.
**	18-Apr-00 (gordy)
**	    Added static data, initializer, getResource() method,
**	    and EmptyRsrc class definition to make class self-
**	    contained.
**	13-Jun-00 (gordy)
**	    Added procedure call syntax error E_JD0016.
**	30-Oct-00 (gordy)
**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
**	31-Jan-01 (gordy)
**	    Added invalid date format E_JD0019_INVALID_DATE.
**	11-Sep-02 (gordy)
**	    Renamed to remove specific product reference.
**	    Translated JDBC error codes into new GCF error codes.
**	26-Dec-02 (gordy)
**	    Added bad character set E_GC4009.
**	14-Feb-03 (gordy)
**	    Added invalid transaction state E_GC401F and missing
**	    parameter E_GC4020.
**	 1-Nov-03 (gordy)
**	    Added invalid row request E_GC4021.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	 5-Dec-07 (gordy)
**	    Added additional server error codes to provide local exceptions:
**	    E_GC480D, E_GC480E, E_GC481E, E_GC481F, E_GC4811.
**	15-Feb-08 (rajus01) SD issue 121381, Bug 119928.
**	    Use more generic term LOB instead of BLOB/CLOB in error messages.
*/

public class
ErrRsrc
    extends ListResourceBundle
{

    public static final String	E_GC4000 = "E_GC4000";
    public static final String	E_GC4001 = "E_GC4001";
    public static final String	E_GC4002 = "E_GC4002";
    public static final String	E_GC4003 = "E_GC4003";
    public static final String	E_GC4004 = "E_GC4004";
    public static final String	E_GC4005 = "E_GC4005";
    public static final String	E_GC4006 = "E_GC4006";
    public static final String	E_GC4007 = "E_GC4007";
    public static final String	E_GC4008 = "E_GC4008";
    public static final String	E_GC4009 = "E_GC4009";
    public static final String	E_GC4010 = "E_GC4010";
    public static final String	E_GC4011 = "E_GC4011";
    public static final String	E_GC4012 = "E_GC4012";
    public static final String	E_GC4013 = "E_GC4013";
    public static final String	E_GC4014 = "E_GC4014";
    public static final String	E_GC4015 = "E_GC4015";
    public static final String	E_GC4016 = "E_GC4016";
    public static final String	E_GC4017 = "E_GC4017";
    public static final String	E_GC4018 = "E_GC4018";
    public static final String	E_GC4019 = "E_GC4019";
    public static final String	E_GC401A = "E_GC401A";
    public static final String	E_GC401B = "E_GC401B";
    public static final String	E_GC401C = "E_GC401C";
    public static final String	E_GC401D = "E_GC401D";
    public static final String	E_GC401E = "E_GC401E";
    public static final String	E_GC401F = "E_GC401F";
    public static final String	E_GC4020 = "E_GC4020";
    public static final String	E_GC4021 = "E_GC4021";
    public static final String	E_GC4022 = "E_GC4022";
    public static final String	E_GC4023 = "E_GC4023";
    public static final String	E_GC480D = "E_GC480D";
    public static final String	E_GC480E = "E_GC480E";
    public static final String	E_GC4811 = "E_GC4811";
    public static final String	E_GC481E = "E_GC481E";
    public static final String	E_GC481F = "E_GC481F";

    private static final Object   info[][] = 
    {
	/*
	** Error messages.
	*/
	{ E_GC4000,	// E_GC4000_BAD_URL
	  "Target connection string improperly formatted." },
	{ E_GC4001,	// E_GC4001_CONNECT_ERR
	  "Communications error while establishing connection." },
	{ E_GC4002,	// E_GC4002_PROTOCOL_ERR
	  "Connection aborted due to communications protocol error." },
	{ E_GC4003,	// E_GC4003_CONNECT_FAIL
	  "Connection failed." },
	{ E_GC4004,	// E_GC4004_CONNECTION_CLOSED
	  "Connection closed." },
	{ E_GC4005,	// E_GC4005_SEQUENCING
	  "New request made before prior request completed." },
	{ E_GC4006,	// E_GC4006_TIMEOUT
	  "Operation cancelled due to time-out." },
	{ E_GC4007,	// E_GC4007_BLOB_IO
	  "I/O error during LOB processing." },
	{ E_GC4008,	// E_GC4008_SERVER_ABORT
	  "Server aborted connection." },
	{ E_GC4009,	// E_GC4009_BAD_CHARSET
	  "Could not map server character set to local character encoding." },

	{ E_GC4010,	// E_GC4010_PARAM_VALUE
	  "Invalid parameter value." },
	{ E_GC4011,	// E_GC4011_INDEX_RANGE
	  "Invalid column or parameter index." },
	{ E_GC4012,	// E_GC4012_INVALID_COLUMN_NAME 
	  "Invalid column or parameter name." },
	{ E_GC4013,	// E_GC4013_UNMATCHED
	  "Unmatched quote, parenthesis, bracket or brace." },
	{ E_GC4014,	// E_GC4014_CALL_SYNTAX
	  "Invalid procedure call syntax." },
	{ E_GC4015,	// E_GC4015_INVALID_OUT_PARAM
	  "Output parameters not allowed with row-producing procedures" },
	{ E_GC4016,	// E_GC4016_RS_CHANGED
	  "Requested ResultSet type, concurrency and/or holdability " +
	  "is not supported." },
	{ E_GC4017,	// E_GC4017_NO_RESULT_SET
	  "Query or procedure does not return a result set." },
	{ E_GC4018,	// E_GC4018_RESULT_SET_NOT_PERMITTED
	  "Query or procedure returns a result set" },
	{ E_GC4019,	// E_GC4019_UNSUPPORTED
	  "Request is not supported." },
	{ E_GC401A,	// E_GC401A_CONVERSION_ERR
	  "Invalid datatype conversion." },
	{ E_GC401B,	// E_GC401B_INVALID_DATE
	  "DATE/TIME value not in standard format." },
	{ E_GC401C,	// E_GC401C_BLOB_DONE
	  "Request to retrieve LOB column which had already been accessed.  " +
	  "LOB columns may only be accessed once." },
	{ E_GC401D,	// E_GC401D_RESULTSET_CLOSED 
	  "ResultSet closed." },
	{ E_GC401E,	// E_GC401E_CHAR_ENCODE 
	  "Invalid character/string encoding." },
	{ E_GC401F,	// E_GC401F_XACT_STATE
	  "Invalid transaction state for requested operation." },
	{ E_GC4020,	// E_GC4020_NO_PARAM
	  "The value for a dynamic parameter was not provided." },
	{ E_GC4021,	// E_GC4021_INVALID_ROW
	  "Cursor is not positioned on a row for which the requested " +
	  "operation is supported." },
	{ E_GC4022,	// E_GC4022_LOB_FREED
	  "LOB object has been freed." },
	{ E_GC4023,	// E_GC4023_NO_OBJECT
	  "No object that implements the given interface." },

	{ E_GC480D,	// E_GC480D_IDLE_LIMIT
	  "Connection aborted by server because idle limit was exceeded." },
	{ E_GC480E,	// E_GC480E_CLIENT_MAX
	  "Server rejected connection request because client limit exceeded." },
	{ E_GC4811,	// E_GC4811_NO_STMT
	  "Statement does not exist.  It may have already been closed." },
	{ E_GC481E,	// E_GC481E_SESS_ABORT
	  "Server aborted connection by administrator request." },
	{ E_GC481F,	// E_GC481F_SVR_CLOSED
	  "Server is not currently accepting connection requests." },
    };

/*
** Name: getContents
**
** Description:
**	Returns an array of the defined key/value pairs.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Object[][]	Key/value pair array.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Changed access to protected to match super-class and restrict
**	    public access to internal info list.
*/

protected Object[][]
getContents()
{
    return( info );
} // getContents


/*
** The following static data, method and class definition 
** are only needed in the resource base class ErrRsrc.
*/

    /*
    ** Base resource class definition for
    ** loading Locale specific resources.
    */
    private static final String		RSRC_CLASS  = 
						"com.ingres.gcf.util.ErrRsrc";

    /*
    ** Locale specific resource.
    */
    private static ResourceBundle   resource = null;


/*
** Name: getResource
**
** Description:
**	Returns the error resource for the default Locale.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResourceBundle	Resource for default Locale.
**
** History:
**	18-Apr-00 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Renamed to remove specific product reference.
*/

public static ResourceBundle
getResource()
{
    if ( resource == null )
    {
	/*
	** Load the Locale specific resources for the default Locale
	** and make sure that a resource bundle (even empty) exists.
	*/
	try { resource = ResourceBundle.getBundle( RSRC_CLASS ); }
	catch( Exception ignore ){ resource = new EmptyRsrc(); }
    }

    return( resource );
}


/*
** Name: EmptyRsrc
**
** Description:
**	ResourceBundle sub-class which provides no resources.
**	This class is used when the actual resource class can
**	not be accessed to avoid a null reference.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
*/

private static class
EmptyRsrc
    extends ListResourceBundle
{
    private static final Object   contents[][] = new Object[0][];

public Object[][]
getContents()
{
    return( contents );
} // getContents


} // class EmptyRsrc

} // class ErrRsrc
