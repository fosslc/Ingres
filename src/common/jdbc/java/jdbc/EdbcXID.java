/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcXID.java
**
** Description:
**	Implements the EDBC JDBC Java driver class: EdbcXID.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	11-Apr-01 (gordy)
**	    Support tracing.
*/



/*
** Name: EdbcXID
**
** Description:
**	EDBC JDBC Java extension driver class which implements 
**	Ingres distributed transaction ID support.
**
**  Constants:
**
**	XID_INGRES		Ingres XID.
**	XID_XA			XA XID.
**
**  Public Methods:
**
**	getType			Returns Transaction ID type: Ingres, XA.
**	getXId			Returns Ingres Transaction ID.
**	getFormatId		Returns the XA Format ID.
**	getGlobalTransactionId	Returns the XA Global Transaction ID.
**	getBranchQualifier	Returns the XA Branch Qualifier.
**	trace			Write XID to trace log.
**
**  Private Data
**
**	type			XID type: Ingres, XA.
**	xid			Ingres Transaction ID.
**	fid			XA Format ID.
**	gtid			XA Global Transaction ID.
**	bqual			XA Branch Qualifier.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	11-Apr-01 (gordy)
**	    Added trace().
*/

public class
EdbcXID
{

    public static final	int	XID_INGRES = 1;
    public static final int	XID_XA = 2;

    protected int	type = 0;
    protected long	xid = 0;
    protected int	fid = 0;
    protected byte	gtid[] = null;
    protected byte	bqual[] = null;


/*
** Name: EdbcXID
**
** Description:
**	Class constructor for an Ingres XID.
**
** Input:
**	xid	Ingres Transaction ID.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
*/

public
EdbcXID( long xid )
{
    this.type = XID_INGRES;
    this.xid = xid;
} // EdbcXID


/*
** Name: EdbcXID
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
**	 2-Apr-01 (gordy)
**	    Created.
*/

public
EdbcXID( int fid, byte gtid[], byte bqual[] )
{
    this.type = XID_XA;
    this.fid = fid;
    this.gtid = gtid;
    this.bqual = bqual;
} // EdbcXID


/*
** Name: getType
**
** Description:
**	Returns the XID type: Ingres or XA.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Transaction type.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
*/

public int
getType()
{
    return( type );
} // getType


/*
** Name: getXId
**
** Description:
**	Returns the Ingres Transaction ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Ingres Transaction ID.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
*/

public long
getXId()
{
    return( xid );
} // getXId


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
*/

public byte[]
getGlobalTransactionId()
{
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
*/

public byte[]
getBranchQualifier()
{
    return( bqual );
} // getBranchQualifier


/*
** Name: trace
**
** Description:
**	Write XID to trace log.
**
** Input:
**	trace	    Internal tracing.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	11-Apr-01 (gordy)
*/

public void
trace( EdbcTrace trace )
{
    switch( type )
    {
    case XID_INGRES :
	trace.write( "II XID: 0x" + Long.toHexString( xid ) );
	break;

    case XID_XA :
	trace.write( "XA XID: FormatId = 0x" + Integer.toHexString( fid ) );
	trace.write( "GlobalTransactionId: " );
	trace.hexdump( gtid, 0, gtid.length );
	trace.write( "BranchQualifier: " );
	trace.hexdump( bqual, 0, bqual.length );
	break;
    }
    return;
} // trace


} // class EdbcXID
