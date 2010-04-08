/*
** Copyright (c) 2001, 2002 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: XaXid.java
**
** Description:
**	Defines class representing an XA Distributed Transaction ID.
**
**  Classes
**
**	XaXid
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Separated XA and Ingres transaction IDs.
**	10-May-05 (gordy)
**	    Protect against external changes to ID values.
*/

import	javax.transaction.xa.Xid;


/*
** Name: XaXid
**
** Description:
**	Class which represents an XA distributed transaction ID.
**	Implements the javax.transaction.xa.Xid interface.
**
**  Interface Methods:
**
**	getFormatId		Returns the XA Format ID.
**	getGlobalTransactionId	Returns the XA Global Transaction ID.
**	getBranchQualifier	Returns the XA Branch Qualifier.
**
**  Public Methods:
**
**	hashCode		Returns a hash code for this Xid.
**	toString		Formatted XA XID.
**	equals			Same as another Xid?
**
**  Private Data
**
**	fid			XA Format ID.
**	gtid			XA Global Transaction ID.
**	bqual			XA Branch Qualifier.
**	hash			Hash code.
**	hex			Hex formatting characters.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Removed Ingres XID data and methods.
*/

public class
XaXid
    implements Xid
{

    private int		fid = 0;		// Format ID
    private byte	gtid[] = null;		// Global Transaction ID
    private byte	bqual[] = null;		// Branch Qualifier
    private int		hash = 0;		// Hash value.

    private static final char	hex[] =		// Hex formatting
    { '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };


/*
** Name: XaXid
**
** Description:
**	Class constructor for an XA XID from another XID.
**
** Input:
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Renamed for XA XID.
*/

public
XaXid( Xid xid )
{
    this( xid.getFormatId(), xid.getGlobalTransactionId(), 
	  xid.getBranchQualifier() );
    return;
} // XaXid


/*
** Name: XaXid
**
** Description:
**	Class constructor for an XA XID.
**
** Input:
**	fid	XA Format ID.
**	gtid	XA Global Transaction ID.
**	bqual	XA Branch Qualifier.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Renamed for XA XID.
**	10-May-05 (gordy)
**	    Copy transaction ID and branch qualifier to protect
**	    against changes to the source arrays.
*/

public
XaXid( int fid, byte gtid[], byte bqual[] )
{
    this.fid = fid;
    this.gtid = new byte[ gtid.length ];
    this.bqual = new byte[ bqual.length ];
    hash = fid;

    for( int i = 0; i < gtid.length; i++ )
    {
	this.gtid[ i ] = gtid[ i ];
	hash = ((hash << 8) | (hash >>> 24)) ^ ((int)gtid[ i ] & 0xff);
    }

    for( int i = 0; i < bqual.length; i++ )
    {
	this.bqual[ i ] = bqual[ i ];
	hash = ((hash << 8) | (hash >>> 24)) ^ ((int)bqual[ i ] & 0xff);
    }
    
    return;
} // XaXid


/*
** Name: getFormatId
**
** Description:
**	Returns the XA Format ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	XA Format ID.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
*/

public int
getFormatId()
{
    return( fid );
} // getFormatId


/*
** Name: getGlobalTransactionId
**
** Description:
**	Returns the XA Global Transaction ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte[]	XA Global Transaction ID.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	10-May-05 (gordy)
**	    Return copy of transaction ID to avoid external changes.
*/

public byte[]
getGlobalTransactionId()
{
    byte gtid[] = new byte[ this.gtid.length ];
    System.arraycopy( this.gtid, 0, gtid, 0, gtid.length );
    return( gtid );
} // getGlobalTransactionId


/*
** Name: getBranchQualifier
**
** Description:
**	Returns the XA Branch Qualifier.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte[]	XA Branch Qualifier.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	10-May-05 (gordy)
**	    Return copy of transaction ID to avoid external changes.
*/

public byte[]
getBranchQualifier()
{
    byte bqual[] = new byte[ this.bqual.length ];
    System.arraycopy( this.bqual, 0, bqual, 0, bqual.length );
    return( bqual );
} // getBranchQualifier


/*
** Name: hashCode
**
** Description:
**	Returns the hash code.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Hash code.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
*/

public int
hashCode()
{
    return( hash );
} // hashCode


/*
** Name: toString
**
** Description:
**	Returns a string identifying this Ingres XID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    XA XID
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

public String
toString()
{
    StringBuffer    buff = new StringBuffer( "XID:XA:" );
    
    buff.append( fid );				// Format ID
    buff.append( ':' );
    
    for( int i = 0; i < gtid.length; i++ )	// Global Transaction ID
    {
	buff.append( hex[ (gtid[i] >> 4) & 0x0F ] );
	buff.append( hex[ gtid[i] & 0x0F ] );
    }

    buff.append( ':' );
    
    for( int i = 0; i < bqual.length; i++ )	// Branch Qualifier
    {
	buff.append( hex[ (bqual[i] >> 4) & 0x0F ] );
	buff.append( hex[ bqual[i] & 0x0F ] );
    }

    return( buff.toString() );
} // toString


/*
** Name: equals
**
** Description:
**	Determine if this Xid is identical to another Xid.
**
** Input:
**	xid	Xid to compare.
**
** Output:
**	None.
**
** Returns:
**	boolean	True if identical, False otherwise.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
*/

public boolean
equals( Object obj )
{
    if ( this == obj )  return( true );
    if ( ! (obj instanceof Xid) )  return( false );

    Xid xid = (Xid)obj;
    if ( fid != xid.getFormatId() )  return( false );

    byte gtid[] = xid.getGlobalTransactionId();
    byte bqual[] = xid.getBranchQualifier();

    if ( this.gtid.length != gtid.length  ||
	 this.bqual.length != bqual.length )  return( false );

    for( int i = 0; i < gtid.length; i++ )
	if ( this.gtid[ i ] != gtid[ i ] )  return( false );

    for( int i = 0; i < bqual.length; i++ )
	if ( this.bqual[ i ] != bqual[ i ] )  return( false );

    return( true );
} // equals


} // class XaXid
