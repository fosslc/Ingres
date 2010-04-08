/*
** Copyright (c) 2001, 2002 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: IngXid.java
**
** Description:
**	Defines class representing an Ingres Distributed Transaction ID.
**
**  Classes
**
**	IngXid
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Separated XA and Ingres transactions IDs.  
*/


/*
** Name: IngXid
**
** Description:
**	Class which represents an Ingres distributed transaction ID.
**
**  Public Methods:
**
**	getID			Returns Ingres Transaction ID.
**	toString		Formatted Ingres XID.
**
**  Private Data
**
**	xid			Ingres Transaction ID.
**
** History:
**	 2-Apr-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Removed XA XID data and methods.
*/

public class
IngXid
{

    /*
    ** Ingres XID is 2 four-byte integers, which maps
    ** nicely to a Java long (eight-byte integer).
    */
    private long	xid = 0;


/*
** Name: IngXid
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
**	31-Oct-02 (gordy)
**	    Renamed for Ingres XID.
*/

public
IngXid( long xid )
{
    this.xid = xid;
} // IngXid


/*
** Name: getID
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
**	31-Oct-02 (gordy)
**	    Renamed for Ingres XID.
*/

public long
getID()
{
    return( xid );
} // getID


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
**	String	    Ingres XID.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

public String
toString()
{
    return( "XID:II:" + Long.toHexString( xid ) );
} // toString


} // class IngXid
