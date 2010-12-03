/*
** Copyright (c) 2007, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: DrvBlob.java
**
** Description:
**	Defines class which implements the JDBC Blob interface
**	for a DBMS Binary LOB Locator.
**
**  Classes:
**
**	DrvBlob
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added enhanced performance access.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
**	16-Nov-10 (gordy)
**	    Added LOB locator streaming access.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.BufferedInputStream;
import	java.sql.Blob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: DrvBlob
**
** Description:
**	JDBC driver class which implements the JDBC Blob interface
**	for a DBMS Binary LOB Locator.
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
**
**  Public Methods:
**
**	get			Retrieve LOB data stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added get() method providing enhanced performance 
**	    in retrieving LOB data under restricted conditions.
**	 4-May-07 (gordy)
**	    Class is not exposed outside package, restrict access.
*/

class	// package access
DrvBlob
    extends DrvLOB
    implements Blob, DbmsConst
{


/*
** Name: DrvBlob
**
** Description:
**	Class constructor.
**
** Input:
**	conn		DBMS connection.
**	locator		DBMS LOB Locator.
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
*/

public
DrvBlob( DrvConn conn, int locator )
{
    super( conn, DBMS_TYPE_LBLOC, locator );

    title = trace.getTraceName() + "-Blob[" + locator + "]";
    tr_id = "DrvBlob[" + locator + "]";
} // DrvBlob


/*
** Name:  free 
**
** Description:
**	This method frees the Blob object and releases the resources that 
**	it holds. The object is invalid once the free  method is called.
**
**	After free has been called, any attempt to invoke a method other than 
**	free will result in a SQLException being thrown. If free is called 
**	multiple times, the subsequent calls to free are treated as a no-op. 
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
**	18-Jun-09 (rajus01)
**	    Implemented.
*/

public void 
free()
throws SQLException
{
    super.free();
    return;
} // free


/*
** Name: length
**
** Description:
**	Returns the length of the LOB data referenced
**	by the locator.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Length of LOB.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public long
length()
    throws SQLException
{
    isValid();
    return( super.length() );
} // length


/*
** Name: getBytes
**
** Description:
**	Extracts a byte array from the LOB data referenced
**	by the locator.
**
** Input:
**	pos	Starting position.
**	length	Length of string.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Extracted byte array.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public byte[]
getBytes( long pos, int length )
    throws SQLException
{
    isValid();
    return( super.getBytes( pos, length, (length <= conn.max_vbyt_len) ) );
} // getBytes


/*
** Name: position
**
** Description:
**	Search LOB data referenced by locator.
**
** Input:
**	pattern	Search pattern.
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
*/

public long
position( byte[] pattern, long start )
    throws SQLException
{
    if ( pattern == null )  throw new NullPointerException();
    isValid();
    long pos = super.position( pattern, start );
    return( (pos >= 1) ? pos : -1 );
} // position


/*
** Name: position
**
** Description:
**	Search LOB data referenced by locator.
**
** Input:
**	pattern	Search pattern.
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
*/

public long
position( Blob pattern, long start )
    throws SQLException
{
    if ( pattern == null )  throw new NullPointerException();
    long pos = -1;
    isValid();

    if ( pattern instanceof DrvLOB  &&  
	 hasSameDomain( (DrvLOB)pattern ) )
	pos = super.position( (DrvLOB)pattern, start );
    else
	pos = super.position( pattern.getBinaryStream(), start );

    return( (pos >= 1) ? pos : -1 );
} // position


/*
** Name: getBinaryStream
**
** Description:
**	Returns a binary InputStream which reads the 
**	LOB data referenced by the locator.
**
**	Requests made to the stream generate individual
**	and independent requests to the DBMS.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Binary stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Set buffer size.  Request a deferred stream.
**	16-Nov-10 (gordy)
**	    LOB locator access can either be done in separate
**	    requests for individual segments or as a single
**	    streaming data request.
*/

public InputStream
getBinaryStream()
    throws SQLException
{
    isValid();

    /*
    ** Generate a byte stream to represent the LOB
    ** and wrap it with a buffer to improve access.
    **
    ** Access can either be streaming or segmented.
    */
    boolean segment = ((conn.cnf_flags & conn.CNF_LOC_STRM) == 0);
    return( new BufferedInputStream( super.getByteStream( segment ), 
				     conn.cnf_lob_segSize ) );
} // getBinaryStream


/*
** Name: getBinaryStream
**
** Description:
**      Returns a binary InputStream that contains a partial Blob value, 
**	starting with the byte specified by pos, which is len bytes in
**      LOB data referenced by the locator.
**
**      Requests made to the stream generate individual
**      and independent requests to the DBMS.
**
** Input:
**      pos  The offset to the first byte of the partial value to be 
**	     retrieved. The first byte in the Blob is at position 1.
**	len  The length in bytes of the partial value to be retrieved
**
** Output:
**      None.
**
** Returns:
**      InputStream     Binary stream.
**
** History:
**      24-Nov-08 (rajus01)
**          Created.
**	18-Jun-09 (rajus01)
**	    Implemented.
**	16-Nov-10 (gordy)
**	    LOB locator access can either be done in separate
**	    requests for individual segments or as a single
**	    streaming data request.
*/

public InputStream 
getBinaryStream(long pos, long len)
throws SQLException
{
    isValid();

    /*
    ** Validate parameters.
    */
    if ( pos < 1  ||  len < 0 )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
 
    /*
    ** Generate a byte stream to represent the LOB
    ** and wrap it with a buffer to improve access.
    **
    ** Access can either be streaming or segmented.
    */
    boolean segment = ((conn.cnf_flags & conn.CNF_LOC_STRM) == 0);
    return( new BufferedInputStream( super.getByteStream( segment, pos, len ), 
				     conn.cnf_lob_segSize ) );
} // getBinaryStream


/*
** Name: get
**
** Description:
**	Retrieves the LOB data associated with the
**	locator and returns an InputStream which
**	may be used to read the data.
**
**	The data stream must be read immediately
**	and completely consumed prior to any other
**	request being made on this Blob or the
**	associated connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Binary stream.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	16-Nov-10 (gordy)
**	    Changed meaning of getByteStream() parameter.  Access is
**	    always deferred until InputStream is accessed.  Access
**	    can be segmented or streaming.  The latter is used here.
*/

public InputStream
get()
    throws SQLException
{
    isValid();
    return( super.getByteStream( false ) );
} // get


/*
** The following methods are not supported due to the
** fact the DBMS LOB Locators are not updatable.
*/

public int
setBytes( long pos, byte bytes[] )
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setBytes

public int
setBytes( long pos, byte bytes[], int offset, int len )
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setBytes

public OutputStream
setBinaryStream( long pos )
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setBinary Stream


public void
truncate( long len )
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // truncate


} // class DrvBlob
