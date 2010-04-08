/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: DbConn.java
**
** Description:
**	EDBC Driver class DbConn representing a connection
**	with the JDBC (and DBMS) server.
**
**	The DbConn* classes present a single unified class,
**	through inheritance from DbConnIo to DbConn, for
**	access to the JDBC server.  They are divided into
**	separate classes to group related functionality for
**	ease in management.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	29-Sep-99 (gordy)
**	    Added methods (lock()/unlock()) and fields to provide
**	    synchronization.
**	 2-Nov-99 (gordy)
**	    Added methods setDbmsInfo() and getDbmsInfo() to DbConn.
**	 4-Nov-99 (gordy)
**	    Added ability to cancel a query.
**	12-Nov-99 (gordy)
**	    Allow gmt or local timezone to support gateways.
**	17-Nov-99 (gordy)
**	    Extracted I/O functionality to DbConnIo.java, DbConnOut.java
**	    and DbConnIn.java.
**	 4-Feb-00 (gordy)
**	    A single thread can dead-lock if it starts another operation
**	    while it already holds the lock.  Save the lock owner and
**	    throw an exception in this case.
**	21-Apr-00 (gordy)
**	    Moved to package io.
**	19-May-00 (gordy)
**	    Added public field for select_loops and make additional
**	    validation check during unlock().
**	19-Oct-00 (gordy)
**	    Added getXactID().
**	 3-Nov-00 (gordy)
**	    Removed timezone fields/methods and replaced with public
**	    boolean field: use_gmt_tz.
**	20-Jan-01 (gordy)
**	    Added msg_protocol_level.
**	 5-Feb-01 (gordy)
**	    Coalesce statement IDs using a statement ID cache with the
**	    query text as key which is cleared at transaction boundaries.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	20-Aug-01 (gordy)
**	    Added support for default cursor mode.
**	20-Feb-02 (gordy)
**	    Added fields for DBMS protocol level and Ingres/gateway distinction.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**       2-Sep-03 (weife01)
**          Bug 110772: Allow non-owner thread to do the unlock.
**	 14-Dec-04 (rajus01) Startrak# EDJDBC94; Bug# 113625
**	     Added abort().
*/

import	java.util.Hashtable;
import	java.util.Random;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Trace;
import	ca.edbc.util.EdbcCI;


/*
** Name: DbConn
**
** Description:
**	EDBC Driver class representing a database connection.  
**	This is the class from the DbConn* group of classes 
**	which is exposed outside the package.  Tracks the state 
**	of the connection and provides methods for reading and 
**	writing messages.
**
**	When reading/writing messages, the caller must provide
**	multi-threaded protection for each message.  Since the 
**	Ingres communication channel is not multi-session, the 
**	caller should provide multi-threaded protection from the 
**	first message sent in a request until the last response 
**	message has been received.  The methods lock() and unlock()
**	permit the caller to synchronize multi-threaded access
**	to the communication channel.
**
**  Public Data:
**
**	CRSR_DBMS	    Cursor mode determined by DBMS.
**	CRSR_READONLY	    Readonly cursor mode.
**	CRSR_UPDATE	    Updatable cursor mode.
**
**	msg_protocol_level  Negotiated message protocol level.
**	use_gmt_tz	    Is connection using GMT timezone?
**	ucs2_supported	    Is UCS2 data supported?
**	select_loops	    Are select loops enabled?
**	cursor_mode	    Default cursor mode.
**
**  Public Methods:
**
**	close		Close the connection.
**	cancel		Cancel current query.
**	lock		Lock connection.
**	unlock		Unlock connection.
**	encode		Encrypt a string using a key.
**	endXact		Declare end of transaction.
**	getUniqueID	Return a new unique identifier.
**	getStmtID	Return a statement ID for a query.
**	setDbmsInfo	Save DBMS information.
**	getDbmsInfo	Retrieve DBMS information.
**
**  Private Data:
**
**	cncl		Cancel output buffer.
**	dbmsInfo	DBMS information keys and values.
**	stmtID		Statement ID cache.
**	tran_id		Transaction ID.
**	obj_id		Object ID.
**	lock_obj	Object used to lock() and unlock().
**	lock_active	Connection is locked.
**	lock_thread	Lock owner.
**
**	KS		Static encryption buffers.
**	kbuff
**	rand
**
**  Private Methods:
**
**	disconnect	Disconnect from the server.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	29-Sep-99 (gordy)
**	    Added methods (lock()/unlock()) and fields to provide
**	    synchronization.
**	 2-Nov-99 (gordy)
**	    Added methods setDbmsInfo() and getDbmsInfo().
**	 4-Nov-99 (gordy)
**	    Added cancel() method and cncl output buffer.
**	12-Nov-99 (gordy)
**	    Added methods to permit configuration of timezone.
**	 4-Feb-00 (gordy)
**	    Added a thread reference for owner fo the current lock.
**	19-May-00 (gordy)
**	    Added public select_loops status field.
**	31-May-00 (gordy)
**	    Make date formatters public.
**	19-Oct-00 (gordy)
**	    Added getXactID().
**	 3-Nov-00 (gordy)
**	    Removed timezone fields/methods and replaced with public
**	    boolean field, use_gmt_tz, indicating if connection uses
**	    GMT timezone (default is to use GMT).
**	20-Jan-01 (gordy)
**	    Added msg_protocol_level now that there are more than 1 level.
**	 5-Feb-01 (gordy)
**	    Added stmtID as statment ID cache for the new method
**	    getStmtID().
**	10-May-01 (gordy)
**	    Added public flag, ucs2_supported, for UCS2 support.
**	20-Aug-01 (gordy)
**	    Added default for connection cursors, cursor_mode, and
**	    related constants: CRSR_DBMS, CRSR_READONLY, CRSR_UPDATE.
**	20-Feb-02 (gordy)
**	    Added db_protocol_level and is_ingres to handle differences
**	    in DBMS protocol levels and gateways.
*/

public class
DbConn 
    extends DbConnIn
{

    /*
    ** Constants.
    */
    public static final int CRSR_DBMS	    = -1;
    public static final int CRSR_READONLY   = 0;
    public static final int CRSR_UPDATE	    = 1;

    /*
    ** Public data for connection status.
    */
    public  byte	    msg_protocol_level = 0;
    public  byte	    db_protocol_level = 0;
    public  boolean	    is_ingres = true;
    public  boolean	    use_gmt_tz = true;
    public  boolean	    ucs2_supported = false;
    public  boolean	    select_loops = false;
    public  int		    cursor_mode = CRSR_DBMS;

    
    /*
    ** Local data.
    */
    private OutBuff	    cncl = null;
    private Hashtable	    dbmsInfo = new Hashtable();
    private Hashtable	    stmtID = new Hashtable();
    private int		    tran_id = 0;
    private int		    obj_id = 0;

    /*
    ** The connection must be locked from the start
    ** of a message until the last of the response.
    ** BLOB handling causes any synchronized method
    ** or block to terminate prematurely, so methods
    ** to lock and unlock the connection are provided.
    ** A lock object is used for synchronization and
    ** a boolean defines the state of the lock.
    */
    private boolean	    lock_active = false;
    private Thread	    lock_thread = null;
    private Object	    lock_obj = new Object();

    /*
    ** Buffers used by encode() (require synchronization).
    */
    private static byte	    KS[] = new byte[ EdbcCI.KS_SIZE ];
    private static byte	    kbuff[] = new byte[ EdbcCI.CRYPT_SIZE ];
    private static Random   rand = new Random();


/*
** Name: DbConn
**
** Description:
**	Class constructor.  Opens a socket connection to target
**	host and initializes the I/O buffers.  Sends the Message
**	layer parameter info byte array (if provided) and returns 
**	any response (array is 0 filled if no response).  The
**	array must be sufficiently long to hold any response
**
** Input:
**	host_id		Hostname and port.
**	info		Message layer parameter info.
**	info_len	Length of parameter info.
**
** Output:
**	info		Response info or 0 filled.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	17-Nov-99 (gordy)
**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.
*/

public
DbConn( String host_id, byte info[], short info_len )
    throws EdbcEx
{
    super( host_id, info, info_len );
    
    try 
    { 
	cncl = new OutBuff( socket.getOutputStream(), conn_id, 16 );
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error creating cancel buffer: " + 
			 ex.getMessage() );
	disconnect();
	throw EdbcEx.get( E_IO0001_CONNECT_ERR, ex );
    }
} // DbConn

/*
** Name: abort
**
** Description:
**	Disconnect from server.
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
**	 14-Dec-04 (rajus01) Startrak# EDJDBC94, Bug# 113625
**	    Created.
*/
public void
abort()
{
    super.disconnect();
    return;
} // abort

/*
** Name: disconnect
**
** Description:
**	Disconnect from server and free all I/O resources.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.
*/

protected void
disconnect()
{
    /*
    ** We don't set the I/O buffer reference to null
    ** here so that we don't have to check it on each
    ** use.  I/O buffer functions will continue to work
    ** until a request results in a stream I/O request,
    ** in which case an exception will be thrown by the
    ** I/O buffer.
    **
    ** We must, however, test the reference for null
    ** since we may be called by the constructor with
    ** a null cancel buffer.
    */
    if ( cncl != null )
    {
	try { cncl.close(); }
	catch( Exception ignore ) {}
    }

    super.disconnect();
    return;
} // disconnect


/*
** Name: close
**
** Description:
**	Close the connection with the server.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.
*/

public synchronized void
close()
{
    if ( ! isClosed() )
    {
	super.close();
	disconnect();
    }
    return;
} // close


/*
** Name: cancel
**
** Description:
**	Issues an interrupt to the JDBC server which
**	will attempt to cancel any active query.
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
**	 4-Nov-99 (gordy)
**	    Created.
*/

public void
cancel()
    throws EdbcEx
{
    try
    {
	synchronized( cncl )
	{
	    if ( trace.enabled( 2 ) )
		trace.write( title + ": sending cancel request" );
	    cncl.begin( JDBC_TL_INT, 0 );
	    cncl.flush();
	}
    }
    catch( EdbcEx ex )
    {
	disconnect();
	throw ex;
    }

    return;
} // cancel


/*
** Name: lock
**
** Description:
**	Set a lock which will block all other threads until the
**	connection is unlocked.  The lock is released by invoking.
**	unlock().  A thread must not invoke lock() twice without
**	invoking unlock() in between.
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
**	 4-Feb-00 (gordy)
**	    A single thread can dead-lock if it starts another operation
**	    while it already holds the lock.  Save the lock owner and
**	    throw an exception in this case.
**	19-May-00 (gordy)
**	    Changed trace messages.
*/

public void
lock()
    throws EdbcEx
{
    synchronized( lock_obj )
    {
	while( lock_active )  
	{
	    if ( lock_thread == Thread.currentThread() )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( title + ".lock(): locked, current " +
				 lock_thread.toString() );
	    	throw EdbcEx.get( E_IO0006_SEQUENCING );
	    }

	    try { lock_obj.wait(); }
	    catch( Exception ignore ) {}
	}

	lock_thread = Thread.currentThread();
	lock_active = true;
	if ( trace.enabled( 5 ) )
	    trace.write( title + ".lock(): owner " + lock_thread.toString() );
    }

    return;
}


/*
** Name: unlock
**
** Description:
**	Clear the lock set by lock() and activate blocked threads.
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
**	19-May-00 (gordy)
**	    Check for additional invalid unlock conditions.
**       2-Sep-03 (weife01)
**          Bug 110772: Allow non-owner thread to do the unlock.
*/

public void
unlock()
{
    synchronized( lock_obj )
    {
	if ( lock_thread == null  ||  ! lock_active )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ".unlock(): unlocked, current " + 
		             Thread.currentThread().toString() );
	    lock_thread = null;
	    lock_active = false;
	}
	else  
	{
            if ( lock_thread != Thread.currentThread() )
            {
                if( trace.enabled( 1 ) )
                    trace.write( title + ".unlock(): not owner, current " + 
                    Thread.currentThread().toString() + ", owner " + 
                    lock_thread.toString() );
            }
	    else if ( trace.enabled( 5 ) )
		trace.write( title + ".unlock(): owner " + 
                             lock_thread.toString() );
	    lock_active = false;
	    lock_thread = null;
	    lock_obj.notify();
	}
    }

    return;
}


/*
** Name: encode
**
** Description:
**	Encode a string using a key.  The key and string are translated
**	to the Server character set prior to encoding.
**
** Input:
**	key	    Encryption key.
**	data	    Data to be encrypted
**
** Output:
**	None.
**
** Returns:
**	byte []	    Encrypted data.
**
** History:
**	22-Sep-99 (gordy)
**	    Created.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public byte []
encode( String key, String data )
    throws EdbcEx
{
    byte buff[];

    try { buff = encode( char_set.getBytes( key ), 
			 char_set.getBytes( data ) ); }
    catch( Exception ex )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    return( buff );
}


/*
** Name: encode
**
** Description:
**	Applies the Driver semantics to convert the data and key 
**	into form acceptable to CI and encodes the data using the 
**	key provided.
**
** Input:
**	key	    Encryption key.
**	data	    Data to be encrypted.
**
** Output:
**	None.
**
** Returns:
**	byte []	    Encrypted data.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Converted parameters to byte arrays for easier processing
**	    here and to support character set/encoding at a higher level.
**	21-Apr-00 (gordy)
**	    Extracted from class EdbcObject.
*/

public static byte []
encode( byte key[], byte data[] )
{
    byte    buff[] = null;
    int	    i, j, m, n, blocks;

    synchronized( KS )
    {
	/*
	** The key schedule is built from a single CRYPT_SIZE
	** byte array.  We use the input key to build the byte
	** array, truncating or duplicating as necessary.
	*/
	for( m = n = 0; m < EdbcCI.CRYPT_SIZE; m++ )
	{
	    if ( n >= key.length )  n = 0;
	    kbuff[ m ] = key[ n++ ];
	}

	EdbcCI.setkey( kbuff, KS );

	/*
	** The data to be encoded must be padded to a multiple
	** of CRYPT_SIZE bytes.  Random data is used for the
	** pad, so for strings the null terminator is included
	** in the data.  A random value is added to each block
	** so that a given key/data pair does not encode the
	** same way twice.  
	**
	** The total number of blocks can be calculated from 
	** the string length plus the null terminator divided 
	** into CRYPT_SIZE blocks (with one random byte): 
	**	((len+1) + (CRYPT_SIZE-2)) / (CRYPT_SIZE-1)
	** which can be simplified as: 
	**	(len / (CRYPT_SIZE - 1) + 1
	*/
	blocks = (data.length / (EdbcCI.CRYPT_SIZE - 1)) + 1;
	buff = new byte[ blocks * EdbcCI.CRYPT_SIZE ];

	for( i = m = n = 0; i < blocks; i++ )
	{
	    buff[ m++ ] = (byte)(rand.nextInt() & 0xff);

	    for( j = 1; j < EdbcCI.CRYPT_SIZE; j++ )
		if ( n < data.length )
		    buff[ m++ ] = data[ n++ ];
		else  if ( n > data.length )
		    buff[ m++ ] = (byte)(rand.nextInt() & 0xff);
		else
		{
		    buff[ m++ ] = 0;	// null terminator
		    n++;
		}
	}

	EdbcCI.encode( buff, 0, buff.length, KS, buff, 0 );
    }

    return( buff );
} // encode


/*
** Name: endXact
**
** Description:
**	Declares the end of a transaction on the connection.
**	Transaction boundaries are used to track objects
**	associated with the connection which are transaction
**	sensitive such as cursor and prepared statements.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	 5-Feb-01 (gordy)
**	    Clear the statement ID cache.
*/

public synchronized void
endXact()
{
    if ( trace.enabled( 4 ) )
	trace.write( title + ": end of transaction" );
    tran_id++;
    obj_id = 0;
    stmtID.clear();
    return;
} // endXact


/*
** Name: getXactID
**
** Description:
**	Returns the current driver internal transaction ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Transaction ID.
**
** History:
**	19-Oct-00 (gordy)
**	    Created.
*/

public int
getXactID()
{
    return( tran_id );
}


/*
** Name: getUniqueID
**
** Description:
**	Generates a unique identifier to represent objects
**	associated with the database connection (cursors and
**	statements).  The unique identifier is a combination
**	of the callers provided prefix a hex representation
**	of sequence information associated with the connection.
**
** Input:
**	prefix	    Prefix for the unique identifier.
**
** Output:
**	None.
**
** Returns:
**	String	    The unique identifier.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public synchronized String
getUniqueID( String prefix )
{
    return( new String( prefix + Integer.toHexString( tran_id ) + 
			"_" + Integer.toHexString( obj_id++ ) ) );
} // uniqueID


/*
** Name: getStmtID
**
** Description:
**	Returns a statement ID for a query.  The statment ID
**	is generated from the prefix using getUniqueID().  For
**	any single query text, the same statement ID will be
**	for each call, as long as the statement ID is valid
**	on the server.
**
** Input:
**	query	Query text.
**	prefix	Statement ID prefix.
**
** Output:
**	None.
**
** Returns:
**	String	Statement ID.
**
** History:
**	 5-Feb-01 (gordy)
**	    Created.
*/

public synchronized String
getStmtID( String query, String prefix )
{
    String stmt;

    if ( (stmt = (String)stmtID.get( query )) == null )
    {
	stmt = getUniqueID( prefix );
	stmtID.put( query, stmt );
    }

    return( stmt );
}

/*
** Name: getStmtID
**
** Description:
**	Returns a statement ID for a query.  
**
** Input:
**	query	Query text.
**
** Output:
**	None.
**
** Returns:
**	String	Statement ID.
**
** History:
**	 6-Mar-04 (rajus01) Startrack # EDBC63 Bug# 112279
**	    Created.
*/

public synchronized String
getStmtID( String query )
{
    return( (String)stmtID.get( query ));
}

/*
** Name: setDbmsInfo
**
** Description:
**	Saves an information pair (key, value) for which the
**	value may be retrieved using the key by getDbmsInfo().
**
** Input:
**	key	Information key.
**	value	Value associated with key.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 2-Nov-99 (gordy)
**	    Created.
*/

public void
setDbmsInfo( String key, String value )
{
    if ( trace.enabled( 3 ) )  
	trace.write( title + ".setDbmsInfo('" + key + "','" + value + "')" );
    dbmsInfo.put( key, value );
    return;
} // setDbmsInfo


/*
** Name: getDbmsInfo
**
** Description:
**	Returns the value associated with a given key.
**
** Input:
**	key	Information key.
**
** Output:
**	None.
**
** Returns:
**	String	Associated value or null.
**
** History:
**	 2-Nov-99 (gordy)
**	    Created.
*/

public String
getDbmsInfo( String key )
{
    return( (String)dbmsInfo.get( key ) );
} // getDbmsInfo

} // class DbConn


