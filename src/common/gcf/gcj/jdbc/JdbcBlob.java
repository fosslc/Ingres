/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcBlob.java
**
** Description:
**	Defines class which implements the JDBC Blob interface.
**
**	Class acts as a wrapper for Blob objects representing both
**	LOB Locators and cached LOBs.
**
**  Classes:
**
**	JdbcBlob
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Support update operations by converting LOB Locators
**	    to local cached Blob.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.sql.Blob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.BufferedBlob;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: JdbcBlob
**
** Description:
**	JDBC driver class which implements the JDBC Blob interface.
**
**	Class acts as a wrapper for Blob objects representing both
**	LOB Locators and cached LOBs.  LOB Locators are not updatable.
**	If an update method is called for an instance which wraps a
**	LOB Locator,  the associated LOB data is cached and subsequent
**	operations are performed on the cached LOB.
**
**  Interface Methods:
**
**	getBinaryStream		Retrieve stream which accesses LOB data.
**	getBytes		Read bytes from LOB data.
**	length			Get length of LOB data.
**	position		Search LOB data.
**	setBinaryStream		Write LOB data from stream.
**	setBytes		Write bytes into LOB data.
**	truncate		Set length of LOB data.
**
**  Protected Methods:
**
**	isValidLocator		Blob represents a LOB Locator.
**	getLOB			Get underlying LOB Locator.
**
**  Private Data:
**
**	blob			Underlying DBMS Blob.
**	segSize			Buffer segment size.
**	trace			Tracing output.
**	title			Class title for tracing.
**
**  Private Methods:
**
**	prepareToUpdate		Convert to updatable.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Added prepareToUpdate() to convert to cached LOB if needed.
**	26-Feb-07 (gordy)
**	    Added segment size and constructor to provide segment size.
*/

public class
JdbcBlob
    implements Blob, DrvConst, GcfErr
{

    private Blob		blob = null;
    private int			segSize = DRV_DFLT_SEGSIZE;
    private DrvTrace		trace = null;
    private String		title = null;


/*
** Name: JdbcBlob
**
** Description:
**	Class constructor.
**
** Input:
**	trace		Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	11-May-09 (rajus01)
**	    Created.
*/

/* Package Access */
JdbcBlob( DrvTrace trace )
{
    this( DRV_DFLT_SEGSIZE, trace );
}

/*
** Name: JdbcBlob
**
** Description:
**	Class constructor.
**
** Input:
**	segSize		Buffered segment size.
**	trace		Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	11-May-09 (rajus01)
**	    Created.
*/

/* Package Access */
JdbcBlob( int segSize, DrvTrace trace )
{
    this.blob = new BufferedBlob( segSize );
    this.trace = trace;
    title = trace.getTraceName() + "-Blob:[cache]";
}

/*
** Name: JdbcBlob
**
** Description:
**	Class constructor.
**
** Input:
**	blob		The underlying DBMS Blob.
**	trace		Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcBlob( DrvBlob blob, DrvTrace trace )
{
    this.blob = blob;
    this.trace = trace;
    title = trace.getTraceName() + "-Blob[LOC:" + blob.toString() + "]";
} // JdbcBlob


/*
** Name: JdbcBlob
**
** Description:
**	Class constructor.
**
** Input:
**	blob		The underlying DBMS Blob.
**	segSize		Buffered segment size.
**	trace		Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcBlob( DrvBlob blob, int segSize, DrvTrace trace )
{
    this( blob, trace );
    this.segSize = segSize;
} // JdbcBlob


/*
** Name: toString
**
** Description:
**	Returns a descriptive name for this object.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Object name.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public String
toString()
{
    return( title );
} // toString


/*
** Name: isValidLocator
**
** Description:
**	Returns an indication that this Blob represents a
**	DBMS LOB Locator associated with the given connection.
**
** Input:
**	conn	Target connection.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if LOB Locator associated with connection.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Make sure underlying Blob is LOB Locator.
*/

boolean	// package access required
isValidLocator( DrvConn conn )
{
    return( (blob instanceof DrvLOB) 
	    ? ((DrvLOB)blob).hasSameDomain( conn ) : false );
} // isValidLocator


/*
** Name: getLOB
**
** Description:
**	Returns the underlying LOB Locator object.  
**
**	If underlying Blob is not a LOB Locator, NULL is returned.
**	This method should only be called when isValidLocator()
**	returns TRUE.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	DrvLOB	LOB Locator or NULL.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Make sure underlying Blob is LOB Locator.
*/

DrvLOB	// package access required
getLOB()
{
    return( (blob instanceof DrvLOB) ? (DrvLOB)blob : null );
} // getLOB


/*
** Name: length
**
** Description:
**	Returns the length of the Blob.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Length of Blob.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public long
length()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".length()" );
    isValid();
    long length = blob.length();
    if ( trace.enabled() )  trace.log( title + ".length: " + length );
    return( length );
} // length


/*
** Name: getBinaryStream
**
** Description:
**	Returns a stream capable of reading the Blob data.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Stream which access Blob data.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public InputStream
getBinaryStream()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getBinaryStream()" );
    isValid();
    return( blob.getBinaryStream() );
} // getBinaryStream


/*
** Name: getBytes
**
** Description:
**	Extract portion of Blob.
**
** Input:
**	pos	Position of bytes to be extracted.
**	length	Number of bytes to be extracted.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Bytes extracted.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public byte[]
getBytes( long pos, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBytes(" + pos + "," + length + ")" );

    isValid();
    byte bytes[] = blob.getBytes( pos, length );

    if ( trace.enabled() )  
	trace.log( title + ".getBytes: " + bytes.length + " bytes" );
    return( bytes );
} // getBytes


/*
** Name: position
**
** Description:
**	Search Blob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Byte pattern to be matched.
**	start	Starting position of search.
**
** Output:
**	None.
**
** Returns:
**	long	Position of match.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public long
position( byte[] pattern, long start )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".position(" + start + ")" );
    isValid();
    long pos = blob.position( pattern, start );
    if ( trace.enabled() )  trace.log( title + ".position: " + pos );
    return( pos );
} // position


/*
** Name: position
**
** Description:
**	Search Blob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Blob pattern to be matched.
**	start	Starting position of search.
**
** Output:
**	None.
**
** Returns:
**	long	Position of match or -1.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public long
position( Blob pattern, long start )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".position(" + pattern + "," + start + ")" );

    isValid();
    /*
    ** If pattern Blob is an instance of this class,
    ** then we need to use the underlying LOB object
    ** directly to allow determination of locator
    ** validity.
    */
    if ( pattern == null )  throw new NullPointerException();
    if ( pattern instanceof JdbcBlob )  pattern = ((JdbcBlob)pattern).blob;
    long pos = blob.position( pattern, start );

    if ( trace.enabled() )  trace.log( title + ".position: " + pos );
    return( pos );
} // position


/*
** Name: setBytes
**
** Description:
**	Writes bytes into Blob at requested position.
**
** Input:
**	pos	Position in Blob to write bytes.
**	bytes	Bytes to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes written.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public int
setBytes( long pos, byte bytes[] )
    throws SQLException
{
    return( setBytes( pos, bytes, 0, bytes.length ) );
} // setBytes


/*
** Name: setBytes
**
** Description:
**	Writes bytes into Blob at requested position.
**
** Input:
**	pos	Position in Blob to write bytes.
**	bytes	Bytes to be written.
**	offset	Offset of bytes to be written.
**	len	Number of bytes to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes written.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public int
setBytes( long pos, byte bytes[], int offset, int len )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBytes(" + pos + "," + len + ")" );

    isValid();
    prepareToUpdate();
    int length = blob.setBytes( pos, bytes, offset, len );

    if ( trace.enabled() )  trace.log( title + ".setBytes: " + length );
    return( length );
} // setBytes


/*
** Name: setBinaryStream
**
** Description:
**	Returns a stream capable of writing to the Blob at
**	the requested position.
**
** Input:
**	pos		Position to write stream.
**
** Output:
**	None.
**
** Returns:
**	OutputStream	Stream which writes to Blob.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public OutputStream
setBinaryStream( long pos )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setBinaryStream(" + pos + ")");
    isValid();
    prepareToUpdate();
    return( blob.setBinaryStream( pos ) );
} // setBinary Stream


/*
** Name: truncate
**
** Description:
**	Truncate Blob to requested length.
**
** Input:
**	len	Length at which Blob should be truncated.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/

public void
truncate( long len )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".truncate(" + len + ")" );
    isValid();
    prepareToUpdate();
    blob.truncate( len );
    return;
} // truncate


/*
** Name: prepareToUpdate
**
** Description:
**	Converts the underlying Blob to be updatable, if necessary.
**
**	LOB Locators are not updatable.  If underlying Blob is a
**	LOB Locator, the associated LOB data is cached and used
**	for subsequent operations.  Otherwise, no change is made.
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
**	 2-Nov-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Set segment size.  Use more efficient access method.
*/

private void
prepareToUpdate()
    throws SQLException
{
    if ( blob instanceof DrvBlob )  
    {
	blob = new BufferedBlob( segSize, ((DrvBlob)blob).get() ); 
	title = trace.getTraceName() + "-Blob[cache]";
    }

    return;
} // prepareToUpdate()

/*
** Name:  free
**
** Description:
**      This method frees the Blob object and releases the resources the
**      resources that it holds. The object is invalid once the free method
**      is called.
**
**      After free has been called, any attempt to invoke a method other than
**      free will result in a SQLException being thrown. If free is called
**      multiple times, the subsequent calls to free are treated as a no-op.
**
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      24-Nov-08 (rajus01)
**          Created.
**	18-Jun-09 (rajus01) SIR 121238
**	    Implemented.
*/

public void 
free()
throws SQLException
{ 
    if ( blob != null )
    {
	blob.free();
	blob = null;
    }

    return;
}

/*
** Name: getBinaryStream
**
** Description:
**      Returns a binary Reader that contains a partial Blob value,
**      starting with the character specified by pos, which is len characters
**      in length.
**
** Input:
**    pos   The offset to the first character of the partial value to be
**          retrieved. The first character in the Blob is at position 1.
**    len   The length in characters of the partial value to be retrieved.
**
** Output:
**      None.
**
** Returns:
**      Reader  Character stream.
**
** History:
**      24-Nov-08 (rajus01)
**          Created.
**	18-Jun-09 (rajus01)
**	    Validate BLOB object.
*/
public InputStream 
getBinaryStream(long pos, long len) 
throws SQLException
{   
    if ( trace.enabled() )  
	trace.log( title + ".getBinaryStream(" + pos +", " + len + ")" );
    isValid();
    return( blob.getBinaryStream( pos, len ) );
} //getBinayStream

/*
** Name: isValid
**
** Description:
**      Checks validity of this BLOB object and throws an
**      exception if it has been freed.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**       18-Jun-09 (rajus01)
**          Created.
*/
private void
isValid()
    throws SQLException
{

    if ( blob == null )  throw SqlExFactory.get( ERR_GC4022_LOB_FREED );
    return;
} // isValid

} // class JdbcBlob

