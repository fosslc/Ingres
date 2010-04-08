/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcClob.java
**
** Description:
**	Defines class which implements the JDBC Clob interface.
**
**	Class acts as a wrapper for Clob objects representing both
**	LOB Locators and cached LOBs.
**
**  Classes:
**
**	JdbcClob
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**          Support update operations by converting LOB Locators
**          to local cached Clob.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	31-Mar-09 (rajus01) SIR 121238
**          Added protected constructor. Public constructors utilize 
**          protected constructor. The Class JdbcNlob will instantiate the 
**          NCLOB objects instead.  
**      11-Jun-09 (rajus01) SIR 121238
**	    Added isValid() to check validity of CLOB object. Implemented 
**	    free() and getCharacterStream(long,long)).
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.sql.Clob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.BufferedClob;
import	com.ingres.gcf.util.BufferedNlob;
import	com.ingres.gcf.util.SqlExFactory;
import  com.ingres.gcf.util.GcfErr;


/*
** Name: JdbcClob
**
** Description:
**	JDBC driver class which implements the JDBC Clob interface.
**
**      Class acts as a wrapper for Clob objects representing both
**      LOB Locators and cached LOBs.  LOB Locators are not updatable.
**      If an update method is called for an instance which wraps a
**      LOB Locator,  the associated LOB data is cached and subsequent
**      operations are performed on the cached LOB.
**
**  Interface Methods:
**
**	getAsciiStream		Retrieve stream which accesses LOB data.
**	getCharacterStream	Retrieve stream which accesses LOB data.
**	getSubString		Read characters from LOB data.
**	length			Get length of LOB data.
**	position		Search LOB data.
**	setAsciiStream		Write LOB data from stream.
**	setCharacterStream	Write LOB data from stream.
**	setString		Write characters into LOB data.
**	truncate		Set length of LOB data.
**
**  Protected Methods:
**
**	isValidLocator		Blob represents a LOB Locator.
**	getLOB			Get underlying LOB Locator.
**
**  Private Data:
**
**	clob			The underlying DBMS Clob.
**	segSize			Buffered segment size.
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
**	31-Mar-09 (rajus01) SIR 121238
**          Added protected constructor. Public constructors utilize 
**          protected constructor. The Class JdbcNlob will instantiate the 
**          NCLOB objects instead.  
*/

public class
JdbcClob
    implements Clob, DrvConst, GcfErr
{

    private Clob		clob;
    private int                 segSize;
    private DrvTrace		trace;
    private String		title;


/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
**
** Input:
**	clob		Underlying DBMS Clob.
**	segSize		Buffered segment size.
**	trace		Tracing output.
**	title		Short class title.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	31-Mar-09 (rajus01)
**	    Created.
*/

protected
JdbcClob( Clob clob, int segSize, DrvTrace trace, String title )
{
    this.clob = clob;
    this.segSize = segSize;
    this.trace = trace;
    this.title = trace.getTraceName() + "-" + title;
} // JdbcClob


/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
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
JdbcClob( DrvTrace trace )
{
    this( DRV_DFLT_SEGSIZE, trace );
}

/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
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
JdbcClob( int segSize, DrvTrace trace )
{
    this( new BufferedClob( segSize ), segSize, trace, "Clob[cache]" );
}


/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
**
** Input:
**	clob	Underlying DBMS Clob.
**	trace	Tracing output.
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
**	31-Mar-09 (rajus01)
**	    Calls the new constructor.
*/

// package access
JdbcClob( DrvClob clob, DrvTrace trace )
{
    this( clob, DRV_DFLT_SEGSIZE, trace );
} // JdbcClob

/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
**
** Input:
**	nlob		Underlying DBMS NClob.
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
**	10-Apr-09 (rajus01)
**	    Created.
*/

// package access
JdbcClob( DrvNlob nlob, DrvTrace trace )
{
    this( nlob, DRV_DFLT_SEGSIZE, trace );
} // JdbcClob

/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
**
** Input:
**	clob		Underlying DBMS Clob.
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
**	31-Mar-09 (rajus01)
**	    Calls the new main constructor.
*/

// package access
JdbcClob( DrvClob clob, int segSize, DrvTrace trace )
{
    this( clob, segSize, trace, "Clob[LOC:" + clob.toString() + "]" );
} // JdbcClob

/*
** Name: JdbcClob
**
** Description:
**	Class constructor for Character LOB Locators.
**
** Input:
**	nlob		Underlying DBMS NClob.
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
**	10-Apr-09 (rajus01)
**	    Created.
*/

// package access
JdbcClob( DrvNlob nlob, int segSize, DrvTrace trace )
{
    this( nlob, segSize, trace, "Nlob[LOC:" + nlob.toString() + "]" );
} // JdbcClob

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
**	Returns an indication that this Clob represents a
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
**          Make sure underlying Blob is LOB Locator.
*/

boolean	// package access required
isValidLocator( DrvConn conn )
{
    return( (clob instanceof DrvLOB) 
	    ? ((DrvLOB)clob).hasSameDomain( conn ) : false );
} // isValidLocator


/*
** Name: getLOB
**
** Description:
**	Returns the underlying LOB Locator object.
**
**      If underlying Clob is not a LOB Locator, NULL is returned.
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
**       2-Feb-07 (gordy)
**          Make sure underlying Blob is LOB Locator.
*/

DrvLOB	// package access required
getLOB()
{
    return( (clob instanceof DrvLOB) ? (DrvLOB)clob : null );
} // getLOB


/*
** Name: length
**
** Description:
**	Returns the length of the Clob.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Length of Clob.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public long
length()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".length()" );
    isValid();
    long length = clob.length();
    if ( trace.enabled() )  trace.log( title + ".length: " + length );
    return( length );
} // length


/*
** Name: getAsciiStream
**
** Description:
**	Returns a stream capable of reading and converting
**	the Clob data into ASCII.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Stream which access Clob data.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public InputStream
getAsciiStream()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getAsciiStream()" );
    isValid();
    return( clob.getAsciiStream() );
} // getAsciiStream


/*
** Name: getCharacterStream
**
** Description:
**	Returns a stream capable of reading the Clob data.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Stream which access Clob data.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public Reader
getCharacterStream()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getCharacterStream()" );
    isValid();
    return( clob.getCharacterStream() );
} // getCharacterStream


/*
** Name: getSubString
**
** Description:
**	Extract portion of Clob.
**
** Input:
**	pos	Position of characters to be extracted.
**	length	Number of characters to be extracted.
**
** Output:
**	None.
**
** Returns:
**	String	Characters extracted.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public String
getSubString( long pos, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSubString(" + pos + "," + length + ")" );

    isValid();
    String str = clob.getSubString( pos, length );

    if ( trace.enabled() )  
	trace.log( title + ".getSubString:" + str.length() + " characters" );
    return( str );
} // getSubString


/*
** Name: position
**
** Description:
**	Search Clob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Character pattern to be matched.
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
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public long
position( String pattern, long start )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".position(" + start + ")" );
    isValid();
    long pos = clob.position( pattern, start );
    if ( trace.enabled() )  trace.log( title + ".position: " + pos );
    return( pos );
} // position


/*
** Name: position
**
** Description:
**	Search Clob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Clob pattern to be matched.
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
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public long
position( Clob pattern, long start )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".position(" + pattern + "," + start + ")" );

    /*
    ** If pattern Clob is an instance of this class,
    ** then we need to use the underlying LOB object
    ** directly to allow determination of locator
    ** validity.
    */
    if ( pattern == null )  throw new NullPointerException();
    if ( pattern instanceof JdbcClob )  pattern = ((JdbcClob)pattern).clob;
    isValid();
    long pos = clob.position( pattern, start );

    if ( trace.enabled() )  trace.log( title + ".position: " + pos );
    return( pos );

} // position


/*
** Name: setString
**
** Description:
**	Writes string into Clob at requested position.
**
** Input:
**	pos	Position in Clob to write string.
**	str	String to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters written.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public int
setString( long pos, String str )
    throws SQLException
{
    return( setString( pos, str, 0, str.length() ) );
} // setString


/*
** Name: setString
**
** Description:
**	Writes characters into Clob at requested position.
**
** Input:
**	pos	Position in Clob to write characters.
**	str	Characters to be written.
**	offset	Offset of characters to be written.
**	len	Number of characters to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters written.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public int
setString( long pos, String str, int offset, int len )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setString(" + pos + "," + len + ")" );

    isValid();
    prepareToUpdate();
    int length = clob.setString( pos, str, offset, len );

    if ( trace.enabled() )  trace.log( title + ".setString: " + length );
    return( length );
} // setString


/*
** Name: setAsciiStream
**
** Description:
**	Returns a stream capable of writing ASCII data
**	to the Clob at the requested position.
**
** Input:
**	pos		Position to write stream.
**
** Output:
**	None.
**
** Returns:
**	OutputStream	Stream which writes to Clob.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public OutputStream
setAsciiStream( long pos )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setAsciiStream(" + pos + ")");
    isValid();
    prepareToUpdate();
    return( clob.setAsciiStream( pos ) );
} // setAsciiStream


/*
** Name: setCharacterStream
**
** Description:
**	Returns a stream capable of writing to the Clob at
**	the requested position.
**
** Input:
**	pos	Position to write stream.
**
** Output:
**	None.
**
** Returns:
**	Writer	Stream which writes to Clob.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Convert to updatable.
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public Writer
setCharacterStream( long pos )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setCharacterStream(" + pos + ")" );
    isValid();
    prepareToUpdate();
    return( clob.setCharacterStream( pos ) );
} // setCharacterStream


/*
** Name: truncate
**
** Description:
**	Truncate Clob to requested length.
**
** Input:
**	len	Length at which Clob should be truncated.
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
**      11-Jun-09 (rajus01)
**          Validate CLOB object.
*/

public void
truncate( long len )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".truncate(" + len + ")" );
    isValid();
    prepareToUpdate();
    clob.truncate( len );
    return;
} // truncate


/*
** Name: prepareToUpdate
**
** Description:
**      Converts the underlying Clob to be updatable, if necessary.
**
**      LOB Locators are not updatable.  If underlying Clob is a
**      LOB Locator, the associated LOB data is cached and used
**      for subsequent operations.  Otherwise, no change is made.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void
**
** History:
**       2-Nov-07 (gordy)
**          Created.
**	26-Feb-07 (gordy)
**	    Set segment size.  Use more effecient access method.
*/

private void
prepareToUpdate()
    throws SQLException
{
    if ( clob instanceof DrvClob )
    {
	clob = new BufferedClob( segSize, ((DrvClob)clob).get() );
	title = trace.getTraceName() + "-Clob[cache]";
    }
    else  if ( clob instanceof DrvNlob )
    {
	clob = new BufferedNlob( segSize, ((DrvNlob)clob).get() );
	title = trace.getTraceName() + "-Nlob[cache]";
    }

    return;
} // prepareToUpdate()


/*
** Name: getCharacterStream
**
** Description:
**      Returns a stream capable of reading the Clob data.
**
** Input:
**    pos       The offset to the first character of the partial value to be
**              retrieved. The first character in the Clob is at position 1.
**    length    The length in characters of the partial value to be retrieved.
**
** Output:
**      None.
**
** Returns:
**      Reader  Stream which accesses Clob data.
**
** History:
**       11-Jun-09 (rajus01) SIR 121238
**          Implemented.
*/

public Reader 
getCharacterStream(long pos, long len)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream( " + pos + "," + len + ")" );
    isValid();
    return( clob.getCharacterStream(pos, len) );
}

/*
** Name: free
**
** Description:
**    This method frees the Clob object and releases the resources that it
**    holds. The object is invalid once the free method is called.
**
**    After free has been called, any attempt to invoke a method other than
**    free will result in a SQLException being thrown. If free is called
**    multiple times, the subsequent calls to free are treated as a no-op.
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
**      11-Jun-09 (rajus01) SIR 121238
**          Implemented.
*/

public void 
free()
throws SQLException
{
    if ( clob != null )
    {
        clob.free();
        clob = null;
    }

    return;
}

/*
** Name: isValid
**
** Description:
**      Checks validity of this CLOB object and throws an
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
**       11-Jun-09 (rajus01)
**          Created.
*/
private void
isValid()
    throws SQLException
{

    if ( clob == null )  throw SqlExFactory.get( ERR_GC4022_LOB_FREED );
    return;
} // isValid

} // class JdbcClob
