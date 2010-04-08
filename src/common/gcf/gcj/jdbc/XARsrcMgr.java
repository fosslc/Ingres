/*
** Copyright (c) 2001, 2006 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: XARsrcMgr.java
**
** Description:
**	Defines interface for JDBC driver XA Resource Manager extensions.
**
**  Interfaces:
**
**	XARsrcMgr
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	10-May-05 (gordy)
**	    Added ability for forcefully abort a distributed transaction.
**	20-Jul-06 (gordy)
**	    Removed XID registration.
**	31-Aug-06 (gordy)
**	    Restored XID registration for start/end operations.
*/


import	java.sql.SQLException;
import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;
import	com.ingres.gcf.util.XaXid;


/*
** Name: XARsrcMgr
**
** Description:
**	JDBC driver interface which supports extensions to the XA
**	Resource Manager interface for requirements which cannot be
**	fully supported by the driver XAResource implementation.
**
**  Methods:
**
**	getRMConnection	Create a Resource Manager connection.
**	registerXID	Register XA resource for XID.
**	deregisterXID	Deregister XA resource.
**	startXID	Start a distributed transaction.
**	endXID		End association with a distributed transaction.
**	prepareXID	Prepare a distributed transaction.
**	commitXID	Commit a distributed transaction.
**	rollbackXID	Rollback a distributed transaction.
**	abortXID	Forcefully abort a distributed transaction.
**	recoverXID	Retrieve list of distributed transaction IDs.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	10-May-05 (gordy)
**	    Added abortXID().
**	20-Jul-06 (gordy)
**	    Removed registerXID() and deregisterXID().  
**	    Added getRMConnection().
**	31-Aug-06 (gordy)
**	    Restored registerXID() and deregisterXID().
**	    Added startXID() and endXID().
*/

interface
XARsrcMgr
{


/*
** Name: getRMConnection
**
** Description:
**	Create a Resource Manager connection.
**
** Input:
**	user		User ID.
**	password	Password.
**
** Output:
**	None.
**
** Returns:
**	JdbcConn	JDBC driver connection.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

JdbcConn getRMConnection( String user, String password ) throws SQLException;


/*
** Name: registerXID
**
** Description:
**	Register a distributed transaction ID and the XAResource
**	object which is servicing the XID.  Any subsequent prepare(),
**	commit(), or rollback() request for the registered XID will
**	be forwarded to the servicing XAResource.
**
** Input:
**	rsrc	XA resource.
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
**	20-Jul-06 (gordy)
**	    Removed.
**	31-Aug-06 (gordy)
**	    Restored.
*/

void registerXID( XAResource rsrc, XaXid xid ) throws XAException;


/*
** Name: deregisterXID
**
** Description:
**	Deregister a previously registered distributed transaction ID.
**
** Input:
**	rsrc	XA resource.
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
**	20-Jul-06 (gordy)
**	    Removed.
**	31-Aug-06 (gordy)
**	    Restored.
*/

void deregisterXID( XAResource rsrc, XaXid xid );


/*
** Name: startXID
**
** Description:
**	Start a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**	flags	XA operational flags.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Aug-06 (gordy)
**	    created.
*/

void startXID( XaXid xid, int flags ) throws XAException;


/*
** Name: endXID
**
** Description:
**	End association with a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**	flags	XA operational flags.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Aug-06 (gordy)
**	    Created.
*/

void endXID( XaXid xid, int flags ) throws XAException;


/*
** Name: prepareXID
**
** Description:
**	Prepare (2PC) a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	int	XA_OK.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
*/

int prepareXID( XaXid xid ) throws XAException;


/*
** Name: commitXID
**
** Description:
**	Commit a Distributed Transaction.
**
** Input:
**	xid	    XA transaction ID.
**	onePhase    Use 1PC protocol?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
*/

void commitXID( XaXid xid, boolean onePhase ) throws XAException;


/*
** Name: rollbackXID
**
** Description:
**	Rollback a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
*/

void rollbackXID( XaXid xid ) throws XAException;


/*
** Name: abortXID
**
** Description:
**	Forcefully abort a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**	server	Server ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	10-May-05 (gordy)
**	    Created.
*/

void abortXID( XaXid xid, String server ) throws XAException;


/*
** Name: recoverXID
**
** Description:
**	Retrieve list of prepared transaction IDs.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Xid[]	Prepared transactions IDs.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Changed driver XA XID class implementation.
*/

Xid[] recoverXID() throws XAException;


} // interface XARsrcMgr
