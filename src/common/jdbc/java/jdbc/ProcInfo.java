/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: ProcInfo.java
**
** Description:
**	Implements the classes to store information about a
**	Database Procedure call request.
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
*/

import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: ProcInfo
**
** Description:
**	Class containing information of a Database Procedure call request.
**
**  Constants:
**
**	PARAM_DEFAULT	    Parameter not specified.
**	PARAM_DYNAMIC	    Parameter dynamically set/retrieved.
**	PARAM_CHAR	    String literal.
**	PARAM_BYTE	    Hexadecimal literal.
**	PARAM_INT	    Integer literal.
**	PARAM_DEC	    Decimal literal.
**	PARAM_FLOAT	    Float literal.
**
**  Public Methods:
**
**	setName		    Set procedure name.
**	getName		    Get procedure name.
**	setReturn	    Set procedure return value expected.
**	getReturn	    Get procedure return value expected.
**	setParam	    Set parameter info.
**	setParamName	    Set parameter name.
**	getParamCount	    Get parameter count.
**	getParamType	    Get parameter type.
**	getParamValue	    Get parameter value.
**	getParamName	    Get parameter name.
**	loadParamNames	    Request parameter names from JDBC server.
**
**  Private Data:
**
**	schema		    Procedure schema.
**	name		    Procedure name.
**	hasRetVal	    Return value expected.
**	ARRAY_INC	    Incremental array size.
**	param_cnt	    Parameter count.
**	params		    Parameter info.
**	rsmd		    Result-set Meta-data.
**	
**  Private Methods:
**
**	paramMap	    Map parameter index.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
*/

class
ProcInfo
    extends EdbcObj
    implements EdbcErr
{
    /*
    ** Parameter types.
    */
    public static final int	PARAM_DEFAULT	= 0;
    public static final int	PARAM_DYNAMIC	= 1;
    public static final int	PARAM_CHAR	= 2;
    public static final int	PARAM_BYTE	= 3;
    public static final int	PARAM_INT	= 4;
    public static final	int	PARAM_DEC	= 5;
    public static final int	PARAM_FLOAT	= 6;
    public static final int	PARAM_SESSION	= 7;

    private static final int	ARRAY_INC = 10;
    private static EdbcRSMD	rsmd = null;

    private String		schema = null;	// Procedure schema
    private String		name = null;	// Procedure name
    private boolean		hasRetVal = false;  // Return value wanted.
    private int			param_cnt = 0;	// Number of parameters
    private Param		params[] = new Param[ ARRAY_INC ];
    private int			current = 0;	// loadParamNames counter.

/*
** Name: ProcInfo
**
** Description:
**	Class constructor.
**
** Input:
**	dbc	Database connection to JDBC server.
**	trace	Connection tracing.
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
*/

ProcInfo( DbConn dbc, EdbcTrace trace )
{
    super( dbc, trace );
    title = shortTitle + "-ProcInfo";
    tr_id = "Proc";
}


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
** Name: setParamName
**
** Description:
**	Save a parameter name.
**
** Input:
**	index	    Parameter index starting with 1.
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
*/

public void
setParamName( int index, String name )
    throws EdbcEx
{
    index = paramMap( index, true );
    params[ index ].name = name;
    return;
} // setParamName


/*
** Name: setParam
**
** Description:
**	Save a parameter type and value.
**
** Input:
**	index	    Parameter index starting with 1.
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
    throws EdbcEx
{
    index = paramMap( index, true );
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
}


/*
** Name: getParamType
**
** Description:
**	Returns a parameter type.
**
** Input:
**	index	    Parameter index starting with 1.
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
    throws EdbcEx
{
    index = paramMap( index, false );
    return( params[ index ].type );
} // getParamType


/*
** Name: getParamValue
**
** Description:
**	Returns a parameter value.
**
** Input:
**	index	    Parameter index starting with 1.
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
    throws EdbcEx
{
    index = paramMap( index, false );
    return( params[ index ].value );
} // getParamValue


/*
** Name: getParamName
**
** Description:
**	Returns a parameter name.
**
** Input:
**	index	    Parameter index starting with 1.
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
    throws EdbcEx
{
    index = paramMap( index, false );
    return( params[ index ].name );
} // getParamType

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
*/

public void
loadParamNames()
    throws EdbcEx
{
    if ( name == null )  return;
    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ": loading procedure parameter names" );

    current = 0;
    dbc.lock();
    try
    {
 	dbc.begin( JDBC_MSG_REQUEST );
	dbc.write( JDBC_REQ_PARAM_NAMES );
	if ( schema != null )
	{
	    dbc.write( (short)JDBC_RQP_SCHEMA_NAME );
	    dbc.write( schema );
	}
	dbc.write( (short)JDBC_RQP_PROC_NAME );
	dbc.write( name );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ": error loading procedure parameter names" );
	    ex.trace( trace );
	}
	throw ex;
    }
    finally
    {
	dbc.unlock();
	warnings = null;
    }

    return;
}


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides the
**	default method in EdbcObject.  It handles the
**	reading of descriptor messages for the method
**	loadParamNames (above).  The data should be in
**	a pre-defined format (see readData() below) so
**	the descriptor is read (by EdbcRSMD) but ignored. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Data descriptor.
**
** History:
**	 13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    if ( rsmd == null )
	rsmd = EdbcRSMD.load( dbc, trace );
    else
	EdbcRSMD.reload( dbc, rsmd );
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of EdbcObj and is called by the readResults
**	method.  It handles the reading of data messages for the
**	loadParamNames() method (above).  The data should be in 
**	a pre-defined format (rows containing 1 non-null varchar 
**	column).
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
*/

protected boolean
readData()
    throws EdbcEx
{
    while( dbc.moreData() )
    {
	dbc.readByte();			// NULL byte (never null)

	/*
	** Read and save parameter name.
	** If there are more names than parameters,
	** the excess is ignored.
	*/
	if ( current < param_cnt )
	    params[ current++ ].name = dbc.readString();
	else
	    dbc.skip( dbc.readShort() );
    }

    return( false );
} // readData


/*
** Name: paramMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the internal index.
**	Ensures the internal parameter array is prepared
**	to hold a parameter for the given index.
**
** Input:
**	index	    External parameter index >= 1.
**	expand	    True if index may reference a new parameter.
**
** Output:
**	None.
**
** Returns:
**	int	    Internal parameter index >= 0.
**
** History:
**	19-May-99 (gordy)
**	    Created.
*/

private int
paramMap( int index, boolean expand )
    throws EdbcEx
{
    if ( index < 1  ||  (index > param_cnt  &&  ! expand) )  
	throw EdbcEx.get( E_JD0007_INDEX_RANGE );

    /*
    ** Expand parameter array if necessary.
    */
    if ( index > params.length )
    {
	int	i, new_max;
	Param	new_array[];

	for( 
	     new_max = params.length + ARRAY_INC; 
	     index > new_max; 
	     new_max += ARRAY_INC
	   );

	new_array = new Param[ new_max ];
	System.arraycopy( params, 0, new_array, 0, params.length );
	params = new_array;
    }

    /*
    ** Adjust parameter counter if index references a new parameter.  
    ** Allocate new parameter objects (if needed) for parameters in
    ** the range not yet initialized.
    */
    for( ; param_cnt < index; param_cnt++ )
	if ( params[ param_cnt ] == null )
	    params[ param_cnt ] = new Param();
    return( index - 1 );
} // paramMap


/*
** Name: Param
**
** Description:
**	Parameter information class.
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

