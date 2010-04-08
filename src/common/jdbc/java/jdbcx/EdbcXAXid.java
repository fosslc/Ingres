/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXAXid.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcXAXid.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
*/


import	javax.transaction.xa.Xid;
import	ca.edbc.jdbc.EdbcXID;


/*
** Name: EdbcXAXid
**
** Description:
**	EDBC JDBC extension class which implements the XA Xid 
**	interface.  Class also supports external Xid classes
**	by providing the ability to act as a cover for any Xid.
**
**	This class provides support for the equals() and hashCode()
**	methods of Object as applied to a Distributed Transaction ID.
**
**  Public Methods:
**
**	equals			Same as another Xid?
**	hashCode		Returns a hash code for this Xid.
**
**  Private Data
**
**	xaXid			XA Xid covered by this XID.
**	hash			Hash code.
**
** History:
**	15-Mar-01 (gordy)
**	    Created.
*/

class
EdbcXAXid
    extends EdbcXID
    implements Xid
{

    private Xid		xaXid = null;
    private int		hash = 0;


/*
** Name: EdbcXAXid
**
** Description:
**	Class constructor as cover for existing Xid.
**
** Input:
**	xid	Existing Xid to cover.
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
*/

public
EdbcXAXid( Xid xid ) 
{
    this( xid.getFormatId(), xid.getGlobalTransactionId(), 
	  xid.getBranchQualifier() );
    this.xaXid = xid;
} // EdbcXAXid


/*
** Name: EdbcXAXid
**
** Description:
**	Class constructor as standalone Xid.
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
*/

public
EdbcXAXid( int fid, byte gtid[], byte bqual[] )
{
    super( fid, gtid, bqual );
    hash = fid;

    for( int i = 0; i < gtid.length; i++ )
	hash = ((hash << 8) | (hash >>> 24)) ^ ((int)gtid[ i ] & 0xff);

    for( int i = 0; i < bqual.length; i++ )
	hash = ((hash << 8) | (hash >>> 24)) ^ ((int)bqual[ i ] & 0xff);
} // EdbcXAXid


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
    Xid xid = (Xid)obj;

    if ( this == xid  ||  this.xaXid == xid )  return( true );
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


} // class EdbcXAXid
