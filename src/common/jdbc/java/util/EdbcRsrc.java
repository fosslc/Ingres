/*
** Copyright (c) 1999, 2004 Ingres Corporation
**
**
*/

package ca.edbc.util;

/*
** Name: EdbcRsrc.java
**
** Description:
**	Locale specific information.  This file defines the
**	default resources values (english,USA) for the EDBC
**	JDBC Driver.  These resources may be localized for
**	different languages and customs by providing suitably
**	named classes (see documentation for ResourceBundle)
**	which define the same keys as this file and provides 
**	the translated values.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Moved to util package.
**	13-Jun-00 (gordy)
**	    Added procedure call syntax error E_JD0016.
**	17-Oct-00 (gordy)
**	    Added autocommit mode definitions.
**	30-Oct-00 (gordy)
**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
**	31-Jan-01 (gordy)
**	    Added invalid date format E_JD0019_INVALID_DATE.
**	28-Mar-01 (gordy)
**	    Renamed effective user resource to DBMS user to
**	    be more generic and pair with DBMS password.
**	20-Aug-01 (gordy)
**	    Added 'Cursor Mode' connection property.
**      29-Aug-01 (loera01) SIR 105641
**          Added invalid output parameter: E_JD0020_INVALID_OUT_PARAM.
**          Added no result set returned: E_JD0021_NO_RESULT_SET.
**          Added result set not permitted: E_JD0022_RESULT_SET_NOT_PERMITTED.
**	03-jan-05 (devjo01)
**	    Correct spelling of "concurrancy", copyright dates & add CA-TOSL.
*/

import	java.util.ResourceBundle;
import	java.util.ListResourceBundle;

/*
** Name: EdbcRsrc
**
** Description:
**	ResourceBundle sub-class which defines the default resource
**	keys and values (english,USA) for the EDBC JDBC Driver.
**
**
**  Public Data:
**
**    Resource keys for Connection Properties:
**
**	cp_db		Database
**	cp_usr		User ID
**	cp_pwd		Password
**	cp_grp		Group ID
**	cp_role		Role ID
**	cp_dbusr	EBMS user ID
**	cp_dbpwd	DBMS password
**	cp_pool		Connection pool
**	cp_xacm		Autocommit mode
**	cp_loop		Select loops
**	cp_crsr		Cursor Mode
**
**    Resource keys for error messages.
**
**	E_JD0001
**	E_JD0002
**	E_JD0003
**	E_JD0004
**	E_JD0005
**	E_JD0006
**	E_JD0007
**	E_JD0008
**	E_JD0009
**	E_JD000A
**	E_JD000B
**	E_JD000C
**	E_JD000D
**	E_JD000E;
**	E_JD000F
**	E_JD0010
**	E_JD0011
**	E_JD0012
**	E_JD0013
**	E_JD0014
**	E_JD0015
**	E_JD0016
**	E_JD0017
**	E_JD0018
**	E_JD0019
**
**  Public methods:
**
**	getContents	Returns the array of keys/values for defined Locale.
**	getResource	Returns the EDBC resources for default Locale
**
**  Private Data:
**
**	info		Array of keys/values.
**	resource	Resource for default Locale.
**
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
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
**	14-Jan-00 (gordy)
**	    Added Connection Pool parameter.
**	25-Jan-00 (rajus01)
**	    Added resultsSet closed: E_JD0013_RESULTSET_CLOSED.
**	31-Jan-00 (rajus01)
**	    Added column not found: E_JD0014_INVALID_COLUMN_NAME.
**	18-Apr-00 (gordy)
**	    Added static data, initializer, getResource() method,
**	    and EmptyRsrc class definition to make class self-
**	    contained.
**	19-May-00 (gordy)
**	    Added connection property for select loops.
**	13-Jun-00 (gordy)
**	    Added procedure call syntax error E_JD0016.
**	17-Oct-00 (gordy)
**	    Added autocommit mode definitions.
**	30-Oct-00 (gordy)
**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
**	31-Jan-01 (gordy)
**	    Added invalid date format E_JD0019_INVALID_DATE.
**	28-Mar-01 (gordy)
**	    Renamed effective user resource to DBMS user to
**	    be more generic and pair with DBMS password.
**	20-Aug-01 (gordy)
**	    Added 'Cursor Mode' connection property.
*/

public class
EdbcRsrc
    extends ListResourceBundle
{

    public static final String	cp_db	= "CP_db";
    public static final String	cp_usr	= "CP_usr";
    public static final String	cp_pwd	= "CP_pwd";
    public static final String	cp_grp	= "CP_grp";
    public static final String	cp_role	= "CP_role";
    public static final String	cp_dbusr= "CP_dbusr";
    public static final String	cp_dbpwd= "CP_dbpwd";
    public static final String	cp_pool	= "CP_pool";
    public static final String	cp_xacm = "CP_xacm";
    public static final	String	cp_loop	= "CP_loop";
    public static final	String	cp_crsr	= "CP_crsr";

    public static final String	E_JD0001 = "E_JD0001";
    public static final String	E_JD0002 = "E_JD0002";
    public static final String	E_JD0003 = "E_JD0003";
    public static final String	E_JD0004 = "E_JD0004";
    public static final String	E_JD0005 = "E_JD0005";
    public static final String	E_JD0006 = "E_JD0006";
    public static final String	E_JD0007 = "E_JD0007";
    public static final String	E_JD0008 = "E_JD0008";
    public static final String	E_JD0009 = "E_JD0009";
    public static final String	E_JD000A = "E_JD000A";
    public static final String	E_JD000B = "E_JD000B";
    public static final String	E_JD000C = "E_JD000C";
    public static final String	E_JD000D = "E_JD000D";
    public static final String	E_JD000E = "E_JD000E";
    public static final String	E_JD000F = "E_JD000F";
    public static final String	E_JD0010 = "E_JD0010";
    public static final String	E_JD0011 = "E_JD0011";
    public static final String	E_JD0012 = "E_JD0012";
    public static final String	E_JD0013 = "E_JD0013";
    public static final String	E_JD0014 = "E_JD0014";
    public static final String	E_JD0015 = "E_JD0015";
    public static final String	E_JD0016 = "E_JD0016";
    public static final String	E_JD0017 = "E_JD0017";
    public static final String	E_JD0018 = "E_JD0018";
    public static final String	E_JD0019 = "E_JD0019";
    public static final String	E_JD0020 = "E_JD0020";
    public static final String	E_JD0021 = "E_JD0021";
    public static final String	E_JD0022 = "E_JD0022";
    public static final String	E_JD0023 = "E_JD0023";

    private static final Object   info[][] = 
    {
	/*
	** Connection properties.
	*/
	{ cp_db,	"Database" },
	{ cp_usr,	"User ID" },
	{ cp_pwd,	"Password" },
	{ cp_grp,	"Group ID" },
	{ cp_role,	"Role ID" },
	{ cp_dbusr,	"DBMS User ID" },
 	{ cp_dbpwd,	"DBMS Password" },
	{ cp_pool,	"Connection Pool" },
	{ cp_xacm,	"Autocommit Mode" },
	{ cp_loop,	"Select Loops" },
	{ cp_crsr,	"Cursor Mode" },

	/*
	** Error messages.
	*/
	{ E_JD0001,	// E_JD0001_CONNECT_ERR
	  "Unable to establish connection due to communications error." },
	{ E_JD0002,	// E_JD0002_PROTOCOL_ERR
	  "Connection aborted due to a communications protocol error." },
	{ E_JD0003,	// E_JD0003_BAD_URL
	  "An improperly formatted URL was provided." },
	{ E_JD0004,	// E_JD0004_CONNECTION_CLOSED
	  "A request was made on a closed connection." },
	{ E_JD0005,	// E_JD0005_CONNECT_FAIL
	  "The connection has failed." },
	{ E_JD0006,	// E_JD0006_CONVERSION_ERR
	  "An invalid datatype conversion was requested." },
	{ E_JD0007,	// E_JD0007_INDEX_RANGE
	  "The column or parameter index is out of range." },
	{ E_JD0008,	// E_JD0008_NOT_QUERY
	  "The SQL statement does not produce a result set." },
	{ E_JD0009,	// E_JD0009_NOT_UPDATE
	  "The SQL statement produces a result set." },
	{ E_JD000A,	// E_JD000A_PARAM_VALUE
	  "Invalid parameter value." },
	{ E_JD000B,	// E_JD000B_BLOB_IO
	  "An I/O error occured during the processing of a BLOB." },
	{ E_JD000C,	// E_JD000C_BLOB_ACTIVE
	  "A request was made to retrieve the value of a column which " +
	  "follows a BLOB column.  BLOB columns must be fully read prior " +
	  "to accessing any subsequent columns." },
	{ E_JD000D,	// E_JD000D_BLOB_DONE
	  "A request was made to retrieve the value of a BLOB column " +
	  "which had already been accessed.  BLOB columns may only be " +
	  "accessed once." },
	{ E_JD000E,	// E_JD000E_UNMATCHED
	  "Query contains an unmatched quote, parenthesis, bracket or brace." },
	{ E_JD000F,	// E_JD000F_TIMEOUT
	  "The requested JDBC operation timed out and was cancelled." },
	{ E_JD0010,	// E_JD0010_CATALOG_UNSUPPORTED
	  "No support for catalogs in identifiers." },
	{ E_JD0011,	// E_JD0011_SQLTYPE_UNSUPPORTED
	  "Unsupported SQL type." },
	{ E_JD0012,	// E_JD0012_UNSUPPORTED
	  "JDBC request is not supported." },
	{ E_JD0013,	// E_JD0013_RESULTSET_CLOSED 
	  "The ResultsSet is closed." },
	{ E_JD0014,	// E_JD0014_INVALID_COLUMN_NAME 
	  "Column not found." },
	{ E_JD0015,	// E_JD0015_SEQUENCING
	  "A new request may not be started while a request is still active." },
	{ E_JD0016,	// E_JD0016_CALL_SYNTAX
	  "Invalid procedure call syntax." },
	{ E_JD0017,	// E_JD0017_RS_CHANGED
	  "Requested ResultSet type and/or concurrency is not supported." },
	{ E_JD0018,	// E_JD0018_UNKNOWN_TIMEZONE
	  "A timezone matching the timezone ID provided could not be found." },
	{ E_JD0019,	// E_JD0019_INVALID_DATE
	  "DATE/TIME value is not in standard JDBC format." },
	{ E_JD0020,	// E_JD0020_INVALID_OUT_PARAM
	  "Output parameters are not permitted on row-producing procedures" },
	{ E_JD0021,	// E_JD0021_NO_RESULT_SET
	  "Procedure does not return a result set" },
	{ E_JD0022,	// E_JD0022_RESULT_SET_NOT_PERMITTED
	  "Result sets are not permitted on update queries" },
	{ E_JD0023,	// E_JD0023_POOLED_CONNECTION_NOT_USABLE
	  "Fatal error occured and pooled connection can no longer be used" },
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
*/

public Object[][]
getContents()
{
    return( info );
} // getContents


/*
** The following static data, method and class definition 
** are only needed in the EDBC resource base class EdbcRsrc.
*/

    private static ResourceBundle   resource = null;


/*
** Name: getResource
**
** Description:
**	Returns the EDBC resource for the default Locale.
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
	try { resource = ResourceBundle.getBundle( "ca.edbc.util.EdbcRsrc" ); }
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

} // class EdbcRsrc
