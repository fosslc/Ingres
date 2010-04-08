/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltBlob.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltBlob
**	for handling BLOBs being recieved on a DB connection.
**
**  Classes
**
**	RsltBlob	BLOB result set.
**	RsltStrm	Base BLOB stream.
**	BinStrm		Binary BLOB stream.
**	AscStrm		ASCII BLOB stream.
**	UniStrm		Unicode BLOB stream.
**	ChrStrm		Character BLOB stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added class to support BLOBs as streams.  Implemented
**	    BLOB data support.  Use DbConn lock()/unlock() methods
**	    for synchronization.
**	18-Nov-99 (gordy)
**	    Made BLOB stream classes nested.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	15-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires unicode stream to be in UTF-8 format.
*/

import	java.io.InputStream;
import	java.io.Reader;
import	java.io.InputStreamReader;
import	java.io.IOException;
import	java.sql.Types;
import	java.sql.SQLException;
import	java.sql.SQLWarning;
import	java.sql.DataTruncation;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: RsltBlob
**
** Description:
**	Abstract EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface portions to support BLOBs being
**	received from a DB connection.
**
**  Abstract Methods:
**
**	load		    Inherited from EdbcRslt.
**	shut		    Inherited from EdbcRslt.
**	resume		    Resume processing of DB results.
**
**  Overridden Methods:
**
**	getBinaryStream	    Retrieve column data as a binary stream.
**	getAsciiStream	    Retrieve column data as an ASCII stream.
**	getUnicodeStream    Retrieve column data as a Unicode stream.
**	getCharacterStream  Retrieve column data as a character stream.
**	closeStrm	    Close an input stream.
**
**  Private Methods
**
**	streamClosed	    Resume query result processing.
**
**  Local Data:
**
**	blob_stream	    Current BLOB stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data and added support
**	    methods and fields.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	 4-Feb-00 (gordy)
**	    Override the closeStrm() method to process columns whose
**	    streams are closed by the superclass such as when a stream
**	    is retrieved through the getString() method.
**	15-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added getCharacterStream()
**	    and ChrStrm class to support the character stream interface.
*/

abstract class
RsltBlob
    extends EdbcRslt
{

    private RsltStrm	    blob_stream = null;


/*
** Name: RsltBlob
**
** Description:
**	Class constructor.
**
** Input:
**	stmt		Associated statment.
**	rsmd		Result set Meta-Data.
**	preFetch	Pre-fetch row count.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Added max row count and max column length.
**	13-Dec-99 (gordy)
**	    Added fetch limit and multi-row data set.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	15-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 support.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
*/

public
RsltBlob
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    int		preFetch
)
    throws EdbcEx
{
    super( stmt, rsmd, preFetch );
    tr_id = "Blob[" + inst_id + "]";
} // RsltBlob


/*
** Abstract classes
*/

protected abstract void	    resume()		throws EdbcEx;


/*
** Name: getBinaryStream
**
** Description:
**	Retrieve column data as an binary stream.  If the column
**	is NULL, null is returned.  Overrides base method to handle
**	the case where the underlying column is a BLOB.  Processing
**	a BLOB results in a partial row fetch.  When the BLOB has
**	been fully read, we must resume the processing of the row.
**	This interaction is handled by a special local class.
**
** Input:
**	columnIndex	Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Binary character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to ResultObj.
*/

public synchronized InputStream
getBinaryStream( int columnIndex )
    throws SQLException
{
    InputStream	value = super.getBinaryStream( columnIndex );

    if ( value != null )
    {
	int index = columnMap( columnIndex );

	switch( rsmd.desc[ index ].sql_type )
	{
	case Types.LONGVARBINARY :
            value = new BinStrm( value);
	    blob_stream = (RsltStrm)value;
	    break;
	}
    }

    return( value );
} // getBinaryStream


/*
** Name: getAsciiStream
**
** Description:
**	Retrieve column data as an ASCII char stream.  If the column
**	is NULL, null is returned.  Overrides base method to handle
**	the case where the underlying column is a BLOB.  Processing
**	a BLOB results in a partial row fetch.  When the BLOB has
**	been fully read, we must resume the processing of the row.
**	This interaction is handled by a special local class.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream ASCII character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to ResultObj.
*/

public synchronized InputStream
getAsciiStream( int columnIndex )
    throws SQLException
{
    InputStream	value = null;;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getAsciiStream( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = getAscii( columnIndex );

    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARCHAR :
	    obj = new AscStrm( (Reader)obj );
	    blob_stream = (RsltStrm)obj;
	    break;

	case Types.LONGVARBINARY :
	    obj = new BinStrm( (InputStream)obj );
	    blob_stream = (RsltStrm)obj;
	    break;
	}

        value = (InputStream)obj;
    }

    if ( trace.enabled() )  trace.log( title + ".getAsciiStream: " + value );
    return( value );
} // getAsciiStream


/*
** Name: getUnicodeStream
**
** Description:
**	Retrieve column data as a Unicode stream.  If the column
**	is NULL, null is returned.  Overrides base method to handle
**	the case where the underlying column is a BLOB.  Processing
**	a BLOB results in a partial row fetch.  When the BLOB has
**	been fully read, we must resume the processing of the row.
**	This interaction is handled by a special local class.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream Unicode character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to ResultObj.
*/

public synchronized InputStream
getUnicodeStream( int columnIndex )
    throws SQLException
{
    InputStream	value = null;;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getUnicodeStream( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = getUnicode( columnIndex );

    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARCHAR :
	    obj = new UniStrm( (Reader)obj );
	    blob_stream = (RsltStrm)obj;
	    break;

	case Types.LONGVARBINARY :
	    obj = new UniStrm( new InputStreamReader( (InputStream)obj ) );
	    blob_stream = (RsltStrm)obj;
	    break;
	}

        value = (InputStream)obj;
    }

    if ( trace.enabled() )  trace.log( title + ".getUnicodeStream: " + value );
    return( value );
} // getUnicodeStream


/*
** Name: getCharacterStream
**
** Description:
**	Retrieve column data as a character stream.  If the column
**	is NULL, null is returned.  Overrides base method to handle
**	the case where the underlying column is a BLOB.  Processing
**	a BLOB results in a partial row fetch.  When the BLOB has
**	been fully read, we must resume the processing of the row.
**	This interaction is handled by a special local class.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Reader	    Character input stream.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized Reader
getCharacterStream( int columnIndex )
    throws SQLException
{
    Reader	value = null;;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = getCharacter( columnIndex );

    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARCHAR :
	    obj = new ChrStrm( (Reader)obj );
	    blob_stream = (RsltStrm)obj;
	    break;

	case Types.LONGVARBINARY :
	    obj = new ChrStrm( new InputStreamReader( (InputStream)obj ) );
	    blob_stream = (RsltStrm)obj;
	    break;
	}

        value = (Reader)obj;
    }

    if ( trace.enabled() )  trace.log( title + ".getCharacterStream: " + value );
    return( value );
} // getCharacterStream


/*
** Name: streamClosed
**
** Description:
**	Resume processing query results following column
**	which interrupted processing.  Currently, only
**	BLOB columns interrupt result processing, so this
**	method should only be called by the BLOB stream
**	when the stream has been completely processed.
**
**	Mark the current stream as done and call sub-class
**	to continue processing.
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
**	29-Sep-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Bump row count once entire row has been read.
**	13-Dec-99 (gordy)
**	    Complete row is now determined by column count
**	    which must be reset for start of new row.
**	25-Jan-00 (gordy)
**	    Added tracing.
*/

private void
streamClosed()
    throws EdbcEx
{
    blob_stream = null;
    resume();		// Call sub-class to continue processing.
    return;
} // streamClosed


/*
** Name: closeStrm
**
** Description:
**	Close the active BLOB stream and process subsequent columns.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Don't need to set the BLOB tracking fields, they
**	    are properly reset by the resume() method.
**	25-Jan-00 (gordy)
**	    Added tracing.
*/

protected void
closeStrm()
    throws EdbcEx
{
    if ( blob_stream != null )
    {
	if ( trace.enabled( 4 ) )
	    trace.write( tr_id + ".closeStrm: flushing column " + cur_col_cnt );
	blob_stream.flush();
    }
    else  if ( cur_col_cnt > 0 )
    {
	int col = cur_col_cnt - 1;

	if ( trace.enabled( 4 ) )  
	    trace.write( tr_id + ".closeStrm: closing column " + cur_col_cnt );

	switch( rsmd.desc[ col ].sql_type )
	{
	case Types.LONGVARCHAR :
	    if ( data_set[ current_row ][ col ] != stream_used )
	    {
		try { ((Reader)data_set[ current_row ][ col ]).close(); }
		catch( Exception ignore ) {}
		data_set[ current_row ][ col ] = stream_used;
	    }
	    break;

	case Types.LONGVARBINARY :
	    if ( data_set[ current_row ][ col ] != stream_used )
	    {
		try { ((InputStream)data_set[ current_row ][ col ]).close(); }
		catch( Exception ignore ) {}
		data_set[ current_row ][ col ] = stream_used;
	    }
	    break;
	}
    }

    streamClosed();
    return;
} // closeStrm

/*
** Name: closeStrm
**
** Description:
**	Close the input stream for the current column.  Overrides
**	the superclass method to resume message processing when
**	the superclass closes an input stream.
**
** Input:
**	type	    Column type (ignored).
**	obj	    Input stream (ignored).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Feb-00 (gordy)
**	    Created.
*/

protected void
closeStrm( int type, Object obj )
    throws EdbcEx
{
    closeStrm();
} // closeStrm


/*
** Name: RsltStrm
**
** Description:
**	BLOB stream access interface.
**
**  Interface Methods:
**
**	flush	    Close stream without notifying result set.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made a nested class (inner) and removed constructor.
*/

private interface
RsltStrm
{

/*
** Name: flush
**
** Description:
**	Interface method which permits the result set to
**	prematurely close the BLOB stream.  Sub-class
**	implementations should not notify the result set
**	of the stream closure caused by this method.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

void flush();

} // class RsltStrm

/*
** Name: BinStrm
**
** Description:
**	Class to access a BLOB InputStream and coordinate with
**	the result set.
**
**  Methods Overriden
**
**	read	    Read stream data.
**	skip	    Skip stream data.
**	available   Number of buffered bytes.
**	close	    Close stream and notify result set.
**	flush	    Close stream without notifying result set.
**
**  Private Data
**
**	in	    BLOB InputStream.
**	closed	    Is stream closed.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made a nested class.
*/

private class 
BinStrm
    extends InputStream
    implements RsltStrm
{

    private	InputStream	in = null;
    private	boolean		closed = false;

/*
** Name: BinStrm
**
** Description:
**	Class constructor.
**
** Input:
**	in	BLOB InputStream.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Removed EdbcResult parameter when made nested class.
*/

public
BinStrm( InputStream in )
{
    this.in = in;
} // BinStrm

/*
** Name: read
**
** Description:
**	Read a single byte value from BLOB stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Value read or -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized int
read()
    throws IOException
{
    int	value = -1;
    if ( ! closed  &&  (value = in.read()) == -1 )  close();
    return( value );
} // read

/*
** Name: read
**
** Description:
**	Read a byte array from BLOB stream.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes to read.
**
** Output:
**	bytes	    Byte array.
**
** Returns:
**	int	    Number of bytes read or -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized int
read( byte bytes[], int offset, int length )
    throws IOException
{
    int len = -1;
    if ( ! closed  &&  (len = in.read( bytes, offset, length )) == -1 )  close();
    return( len );
} // read

/*
** Name: skip
**
** Description:
**	Skip bytes in BLOB stream.
**
** Input:
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of bytes skipped
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized long
skip( long length )
    throws IOException
{
    long len = 0;
    if ( ! closed )  len = in.skip( length );
    return( len );
} // skip

/*
** Name: available
**
** Description:
**	Number of buffered bytes in BLOB stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of bytes available.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized int
available()
    throws IOException
{
    int len = 0;
    if ( ! closed )  len = in.available();
    return( len );
} // available

/*
** Name: close
**
** Description:
**	Close BLOB stream and notify result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized void
close()
    throws IOException
{
    if ( ! closed )
    {
	flush();
	try { RsltBlob.this.streamClosed(); }
	catch( EdbcEx ex )
	{ throw new IOException( ex.getMessage() ); }
    }

    return;
} // close

/*
** Name: flush
**
** Description:
**	Close BLOB stream without notifying result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized void
flush()
{
    if ( ! closed )
    {
	closed = true;
	try { in.close(); }
	catch( Exception ignore ) {}
    }

    return;
} // flush

} // class BinStrm

/*
** Name: AscStrm
**
** Description:
**	Class to access a BLOB Reader stream and (optionally) 
**	coordinate with the result set.
**
**  Methods Overriden
**
**	read	    Read stream data.
**	skip	    Skip stream data.
**	close	    Close stream and notify result set.
**	flush	    Close stream without notifying result set.
**
**  Protected Data
**
**	in	    BLOB Reader stream.
**	closed	    Is stream closed.
**	buffer	    Intermediate character buffer.
**	buff_pos    Offset in buffer.
**	buff_len    Number of characters in buffer.
**	ba	    Temporary storage.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made a nested class.
*/

private class 
AscStrm
    extends InputStream
    implements RsltStrm
{

    protected	Reader		in = null;
    protected	boolean		closed = false;
    protected	char		buffer[] = new char[ 8192 ];
    protected	int		buff_pos = 0;
    protected	int		buff_len = 0;
    protected	byte		ba[] = new byte[1];

/*
** Name: AscStrm
**
** Description:
**	Class constructor.
**
** Input:
**	in	BLOB Reader.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Removed EdbcResult parameter when made nested class.
*/

public
AscStrm( Reader in )
{
    this.in = in;
} // AscStrm

/*
** Name: read
**
** Description:
**	Read a single byte value from BLOB stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Value read or -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized int
read()
    throws IOException
{
    return( read( ba, 0, 1 ) > 0 ? (int)ba[ 0 ] & 0xff : -1 );
} // read

/*
** Name: read
**
** Description:
**	Read a byte array from BLOB stream.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes to read.
**
** Output:
**	bytes	    Byte array.
**
** Returns:
**	int	    Number of bytes read or -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized int
read( byte bytes[], int offset, int length )
    throws IOException
{
    int len = 0;

    if ( closed )  return( -1 );
    if ( length <= 0 )  return( 0 );

    while( len < length )
	if ( buff_len > 0 )
	{
	    while( len < length  &&  buff_len > 0 )
	    {
		bytes[ offset++ ] = (byte)buffer[ buff_pos++ ];
		len++;
		buff_len--;
	    }
	}
	else  if ( (buff_len = in.read( buffer, buff_pos = 0, buffer.length )) < 0 )
	{
	    close();
	    break;
	}

    return( len > 0 ? len : -1 );
} // read

/*
** Name: skip
**
** Description:
**	Skip bytes in BLOB stream.
**
** Input:
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of bytes skipped
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized long
skip( long length )
    throws IOException
{
    long    len = 0;

    if ( closed  ||  length <= 0 )  return( 0L );
    
    while( len < length )
	if ( buff_len > 0 )
	{
	    int cnt = (int)Math.min( (long)buff_len, length - len );
	    len += cnt;
	    buff_len -= cnt;
	    buff_pos += cnt;
	}
	else  if ( (buff_len = in.read( buffer, buff_pos = 0, 
					buffer.length )) < 0 )  
	{
	    close();
	    break;
	}

    return( len );
} // skip

/*
** Name: close
**
** Description:
**	Close BLOB stream and notify result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized void
close()
    throws IOException
{
    if ( ! closed )
    {
	flush();
	try { RsltBlob.this.streamClosed(); }
	catch( EdbcEx ex )
	{ throw new IOException( ex.getMessage() ); }
    }

    return;
} // close

/*
** Name: flush
**
** Description:
**	Close BLOB stream without notifying result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public synchronized void
flush()
{
    if ( ! closed )
    {
	closed = true;
	buffer = null;
	buff_len = buff_pos = 0;
	try { in.close(); }
	catch( Exception ignore ) {}
    }

    return;
} // flush

} // class AscStrm

/*
** Name: UniStrm
**
** Description:
**	Class to access a BLOB Reader stream and coordinate with 
**	the result set.
**
**  Methods Overriden
**
**	read	    Read stream data.
**	skip	    Skip stream data.
**
**  Private Data
**
**	utf8	    Buffer for UTF-8 converted stream.
**	utf8_pos    Position in converted stream.
**	utf8_len    Length of converted stream.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made a nested class.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires unicode stream to be in UTF-8 format.
*/

private class 
UniStrm
    extends AscStrm
{

    private byte	utf8[] = null;
    private int		utf8_pos = 0;
    private int		utf8_len = 0;


/*
** Name: UniStrm
**
** Description:
**	Class constructor.
**
** Input:
**	in	BLOB Reader.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Removed EdbcResult parameter when made nested class.
*/

public
UniStrm( Reader in )
{
    super( in );
} // UniStrm

/*
** Name: read
**
** Description:
**	Read a byte array from BLOB stream.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes to read.
**
** Output:
**	bytes	    Byte array.
**
** Returns:
**	int	    Number of bytes read or -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires unicode stream to be in UTF-8 format.
*/

public synchronized int
read( byte bytes[], int offset, int length )
    throws IOException
{
    int value, len = 0;

    if ( closed )  return( -1 );
    if ( length <= 0 )  return( 0 );

    while( len < length )
	if ( utf8_len > 0 )
	{
	    int seg_len = Math.min( length - len, utf8_len );
	    System.arraycopy( utf8, utf8_pos, bytes, offset, seg_len );
	    utf8_pos += seg_len;
	    utf8_len -= seg_len;
	    offset += seg_len;
	    len += seg_len;
	}
	else  
	{
	    buff_len = in.read( buffer, buff_pos = 0, buffer.length );  
	    if ( buff_len < 0 )  
	    {
		close();
		break;
	    }

	    if ( buff_len > 0 )
	    {
		try { utf8 = ucs2utf8( buffer, 0, buff_len ); }
		catch( SQLException ex )
		{ 
		    close();
		    throw new IOException(); 
		}

		utf8_pos = 0;
		utf8_len = utf8.length;
	    }
	}

    return( len > 0 ? len : -1 );
} // read

/*
** Name: skip
**
** Description:
**	Skip bytes in BLOB stream.
**
** Input:
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of bytes skipped
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires unicode stream to be in UTF-8 format.
*/

public synchronized long
skip( long length )
    throws IOException
{
    long    len = 0;

    if ( closed  ||  length <= 0 )  return( 0L );
    
    while( len < length )
	if ( utf8_len > 0 )
	{
	    int seg_len = Math.min( (int)(length - len), utf8_len );
	    utf8_pos += seg_len;
	    utf8_len -= seg_len;
	    len += seg_len;
	}
	else  
	{
	    buff_len = in.read( buffer, buff_pos = 0, buffer.length );  
	    if ( buff_len < 0 )  
	    {
		close();
		break;
	    }

	    if ( buff_len > 0 )
	    {
		try { utf8 = ucs2utf8( buffer, 0, buff_len ); }
		catch( SQLException ex )
		{ 
		    close();
		    throw new IOException(); 
		}

		utf8_pos = 0;
		utf8_len = utf8.length;
	    }
	}

    return( len );
} // skip

} // class UniStrm


/*
** Name: ChrStrm
**
** Description:
**	Class to access a BLOB Reader stream and (optionally) 
**	coordinate with the result set.
**
**  Methods Overriden
**
**	read	    Read stream data.
**	skip	    Skip stream data.
**	close	    Close stream and notify result set.
**	flush	    Close stream without notifying result set.
**
**  Protected Data
**
**	in	    BLOB Reader stream.
**	closed	    Is stream closed?
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

private class
ChrStrm
    extends Reader
    implements RsltStrm
{

    protected	Reader		in = null;
    protected	boolean		closed = false;

/*
** Name: ChrStrm
**
** Description:
**	Class constructor.
**
** Input:
**	in	BLOB Reader.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public
ChrStrm( Reader in )
{
    this.in = in;
} // ChrStrm


/*
** Name: read
**
** Description:
**	Read a single character from BLOB stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Value read or -1 at end-of-data.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized int
read()
    throws IOException
{
    if ( closed )  return( -1 );
    return( in.read() );
} // read


/*
** Name: read
**
** Description:
**	Read a character array from BLOB stream.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes to read.
**
** Output:
**	bytes	    Byte array.
**
** Returns:
**	int	    Number of bytes read or -1 at end-of-data.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized int
read( char cbuf[], int offset, int length )
    throws IOException
{
    if ( closed )  return( -1 );
    return( in.read( cbuf, offset, length ) );
} // read


/*
** Name: skip
**
** Description:
**	Skip bytes in BLOB stream.
**
** Input:
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of bytes skipped
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized long
skip( long length )
    throws IOException
{
    if ( closed )  return( 0L );
    return( in.skip( length ) );
} // skip


/*
** Name: close
**
** Description:
**	Close BLOB stream and notify result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized void
close()
    throws IOException
{
    if ( ! closed )
    {
	flush();
	try { RsltBlob.this.streamClosed(); }
	catch( EdbcEx ex )
	{ throw new IOException( ex.getMessage() ); }
    }

    return;
} // close


/*
** Name: flush
**
** Description:
**	Close BLOB stream without notifying result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public synchronized void
flush()
{
    if ( ! closed )
    {
	closed = true;
	try { in.close(); }
	catch( Exception ignore ) {}
    }

    return;
} // flush

} // class ChrStrm

} // class RsltBlob

