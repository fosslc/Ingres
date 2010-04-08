/*
** Copyright (c) 1999, 2003 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.jdbc;

/*
** Name: ProcInfo.java
**
** Description:
**	Defines classes for storing information about a Database
**	Procedure call.
**
**  Classes:
**
**	ProcInfo	Procedure information.
**	Param		Parameter information.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Database connection and tracing now constructor parameters.
**	16-Apr-01 (gordy)
**	    Optimize usage of Result-set Meta-data.
**      11-Jul-01 (loera01) SIR 105309
**          Added support for Ingres syntax.
**      13-Aug-01 (loera01) SIR 105521
**          Added PARAM_SESSION constant to support global temp table 
**          parameters.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	24-Feb-03 (gordy)
**	    Changed external index to 0 based.  Track state of param names.
**	 6-Jun-03 (gordy)
**	    Added method to clear parameter info.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: ProcInfo
**
** Description:
**	Class which stores information for a Database Procedure call.
**
**  Constants:
**
**	PARAM_DEFAULT	    Parameter not specified.
**	PARAM_DYNAMIC	    Parameter dynamically set/retrieved.
**	PARAM_SESSION	    Global Temp Table
**	PARAM_CHAR	    String literal.
**	PARAM_BYTE	    Hexadecimal literal.
**	PARAM_INT	    Integer literal.
**	PARAM_DEC	    Decimal literal.
**	PARAM_FLOAT	    Float literal.
**
**  Public Methods:
**
**	setSchema	    Set schema name.
**	getSchema	    Get schema name.
**	setName		    Set procedure name.
**	getName		    Get procedure name.
**	setReturn	    Set procedure return value expected.
**	getReturn	    Get procedure return value expected.
**	clearParams	    Clear parameter info.
**	setParam	    Set parameter info.
**	setParamName	    Set parameter name.
**	getParamCount	    Get parameter count.
**	getParamType	    Get parameter type.
**	getParamValue	    Get parameter value.
**	getParamName	    Get parameter name.
**	paramNamesLoaded    Are parameter names available?
**	loadParamNames	    Request parameter names from JDBC server.
**
**  Private Data:
**
**	ARRAY_INC	    Incremental array size.
**	schema		    Procedure schema.
**	name		    Procedure name.
**	hasRetVal	    Return value expected.
**	param_cnt	    Parameter count.
**	params		    Parameter info.
**
**	haveParamNames	    Are parameter names available?
**	current		    Counter for loadParamNames().
**	rsmd		    Result-set Meta-data.
**	
**  Private Methods:
**
**	readDesc	    Overrides base class for DESCR message.
**	readData	    Overrides base class for DATA message.
**	check		    Validate parameter index.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	24-Feb-03 (gordy)
**	    Added haveParamNames and paramNamesLoaded().  Renamed
**	    paramMap() to check().
**	 6-Jun-03 (gordy)
**	    added clearParams().
*/

class
ProcInfo
    extends DrvObj
    implements GcfErr
{
    /*
    ** Constants
    */
    public static final int	PARAM_DEFAULT	= 0;	// Parameter types
    public static final int	PARAM_DYNAMIC	= 1;
    public static final int	PARAM_CHAR	= 2;
    public static final int	PARAM_BYTE	= 3;
    public static final int	PARAM_INT	= 4;
    public static final	int	PARAM_DEC	= 5;
    public static final int	PARAM_FLOAT	= 6;
    public static final int	PARAM_SESSION	= 7;

    private static final int	ARRAY_INC = 10;

    /*
    ** Procedure and parameter info.
    */
    private String		schema = null;	// Procedure schema
    private String		name = null;	// Procedure name
    private boolean		hasRetVal = false;  // Return value wanted.
    private int			param_cnt = 0;	// Number of parameters
    private Param		params[] = new Param[ ARRAY_INC ];

    /*
    ** Data for loadParamNames().
    */
    private static JdbcRSMD	rsmd = null;
    private boolean		haveParamNames = false;
    private int			current = 0;	// parameter counter.


/*
** Name: ProcInfo
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Associated connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Added database connection and tracing parameters.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

ProcInfo( DrvConn conn )
{
    super( conn );
    title = trace.getTraceName() + "-ProcInfo[" + inst_id + "]";
    tr_id = "ProcInfo[" + inst_id + "]";
    return;
} // ProcInfo


/*
** Name: setSchema
**
** Description:
**	Save the schema of the procedure.
**
** Input:
**	schema	    Procedure schema.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public void
setSchema( String schema )
{
    this.schema = schema;
} // setSchema


/*
** Name: getSchema
**
** Description:
**	Returns the procedure schema.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Procedure schema or NULL.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public String
getSchema()
{
    return( schema );
} // getSchema


/*
** Name: setName
**
** Description:
**	Save the name of the procedure.
**
** Input:
**	name	    Procedure name.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public void
setName( String name )
{
    this.name = name;
} // setName


/*
** Name: getName
**
** Description:
**	Returns the procedure name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Procedure name or NULL.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public String
getName()
{
    return( name );
} // getName


/*
** Name: setReturn
**
** Description:
**	Save an indication if a procedure return value is expected.
**
** Input:
**	hasRetVal	True if return value expected.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public void
setReturn( boolean hasRetVal )
{
    this.hasRetVal = hasRetVal;
    return;
} // setReturn


/*
** Name: getReturn
**
** Description:
**	Returns indication if a procedure return value is expected.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if return value expected.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public boolean
getReturn()
{
    return( hasRetVal );
} // getReturn


/*
** Name: clearParams
**
** Description:
**	Clear parameter information.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 6-Jun-03 (gordy)
**	    Created.
*/

public void
clearParams()
{
    param_cnt = 0;
    haveParamNames = false;
    return;
} // clearParams


/*
** Name: setParamName
**
** Description:
**	Save a parameter name.
**
** Input:
**	index	    Parameter index (0 based).
**	name	    Parameter name.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	11-Jul-01 (loera01) SIR 105309
**	    Created.
**	24-Feb-03 (gordy)
**	    Since positional and named parameters can't be mixed,
**	    setting any name implies that there is no need to load
**	    parameter names from the DBMS.
*/

public void
setParamName( int index, String name )
    throws SQLException
{
    check( index, true );
    params[ index ].name = name;
    haveParamNames = true;
    return;
} // setParamName


/*
** Name: setParam
**
** Description:
**	Save a parameter type and value.
**
** Input:
**	index	    Parameter index (0 based).
**	type	    Parameter type (PARAM_DEFAULT, etc).
**	value	    Parameter value, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public void
setParam( int index, int type, Object value )
    throws SQLException
{
    check( index, true );
    params[ index ].type = type;
    params[ index ].value = value;
    return;
} // setParam


/*
** Name: getParamCount
**
** Description:
**	Returns the number of parameters.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Parameter count.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public int
getParamCount()
{
    return( param_cnt );
} // getParamCount


/*
** Name: getParamType
**
** Description:
**	Returns a parameter type.
**
** Input:
**	index	    Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	int	    Parameter type.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public int
getParamType( int index )
    throws SQLException
{
    check( index, false );
    return( params[ index ].type );
} // getParamType


/*
** Name: getParamValue
**
** Description:
**	Returns a parameter value.
**
** Input:
**	index	    Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	Object	    Parameter value, may be NULL.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public Object
getParamValue( int index )
    throws SQLException
{
    check( index, false );
    return( params[ index ].value );
} // getParamValue


/*
** Name: getParamName
**
** Description:
**	Returns a parameter name.
**
** Input:
**	index	    Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	String	    Parameter name or NULL.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

public String
getParamName( int index )
    throws SQLException
{
    check( index, false );
    return( params[ index ].name );
} // getParamType


/*
** Name: paramNamesLoaded
**
** Description:
**	Returns indication that parameter names have already been
**	loaded and the loadParamNames() method does not need to be
**	called.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if parameter names are loaded.
**
** History:
**	24-Feb-03 (gordy)
**	    Created.
*/

public boolean
paramNamesLoaded()
{
    return( haveParamNames );
} // paramNamesLoaded


/*
** Name: loadParamNames
**
** Description:
**	Requests the names of the procedure parameters
**	from the JDBC server.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Removed database connection parameter.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	24-Feb-03 (gordy)
**	    Mark parameter names as loaded.  Check additional
**	    conditions to skip loading.
*/

public void
loadParamNames()
    throws SQLException
{
    /*
    ** Check if there are no parameters or names have already been
    ** loaded (or set).  Procedure name is required to load params.
    */
    if ( param_cnt < 1  ||  haveParamNames  ||  name == null )
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ": skip loading parameter names (" +
			 param_cnt + "," + haveParamNames + "," + name + ")" );
	return;
    }

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ": loading procedure parameter names" );

    haveParamNames = true;	// Things won't get any better if fails.
    current = 0;		// Init parameter counter.

    msg.lock();
    try
    {
 	msg.begin( MSG_REQUEST );
	msg.write( MSG_REQ_PARAM_NAMES );
	
	if ( schema != null )
	{
	    msg.write( (short)MSG_RQP_SCHEMA_NAME );
	    msg.write( schema );
	}
	
	msg.write( (short)MSG_RQP_PROC_NAME );
	msg.write( name );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ": error loading procedure parameter names" );
	    SqlExFactory.trace( ex, trace );
	}
	throw ex;
    }
    finally
    {
	msg.unlock();
	clearResults();
    }

    return;
} // loadParamNames


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides the default method 
**	in DrvObj.  It handles the reading of descriptor messages for 
**	the method loadParamNames (above).  The data should be in a
**	pre-defined format (see readData() below) so the descriptor 
**	is read (by JdbcRSMD) but ignored. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Data descriptor.
**
** History:
**	 13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    if ( rsmd == null )
	rsmd = JdbcRSMD.load( conn );
    else
	rsmd.reload( conn );
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the default method 
**	of DrvObj and is called by the readResults method.  It handles 
**	the reading of data messages for the loadParamNames() method 
**	(above).  The data should be in a pre-defined format (rows 
**	containing 1 non-null varchar column).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

protected boolean
readData()
    throws SQLException
{
    while( msg.moreData() )
    {
	msg.readByte();			// NULL byte (never null)

	/*
	** Read and save parameter name.
	** If there are more names than parameters,
	** the excess is ignored.
	*/
	if ( current < param_cnt )
	    params[ current++ ].name = msg.readString();
	else
	    msg.skip( msg.readShort() );
    }

    return( false );
} // readData


/*
** Name: check
**
** Description:
**	Determines if a given parameter index is valid.
**	Ensures the internal parameter array is prepared
**	to hold a parameter for the given index.
**
** Input:
**	index	    Parameter index (0 based).
**	expand	    True if index may reference a new parameter.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	24-Feb-03 (gordy)
**	    Renamed to support 0 based parameter indexes.
*/

private synchronized void
check( int index, boolean expand )
    throws SQLException
{
    if ( index < 0  ||  (index >= param_cnt  &&  ! expand) )  
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    /*
    ** Expand parameter array if necessary.
    */
    if ( index >= params.length )
    {
	int new_max;

	for( 
	     new_max = params.length + ARRAY_INC; 
	     index >= new_max; 
	     new_max += ARRAY_INC
	   );

	Param new_array[] = new Param[ new_max ];
	System.arraycopy( params, 0, new_array, 0, params.length );
	params = new_array;
    }

    /*
    ** Adjust parameter counter if index references a new parameter.  
    ** Allocate new parameter objects (if needed) for parameters in
    ** the range not yet initialized.
    */
    for( ; param_cnt <= index; param_cnt++ )
	if ( params[ param_cnt ] == null )  params[ param_cnt ] = new Param();
    
    return;
} // check


/*
** Name: Param
**
** Description:
**	Class representing a procedure parameter.
**
**  Public data
**
**	type	    Parameter type.
**	value	    Parameter value.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

private static class
Param
{

    public  int		type = PARAM_DEFAULT;
    public  Object	value = null;
    public  String	name = null;


} // class Param

} // class ProcInfo

