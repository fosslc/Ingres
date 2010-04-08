/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXARM.java
**
** Description:
**	Defines the EDBC JDBC extension interface EdbcXARM.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
*/


import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;


/*
** Name: EdbcXARM
**
** Description:
**	EDBC JDBC extension interface definition.  This interface 
**	defines an XA Resource Manager interface which supports 
**	the XA requirements which cannot be fully supported 
**	through the EDBC XAResource implementation.
**
**  Methods:
**
**	registerXID	Register XA resource for XID.
**	deregisterXID	Deregister resource.
**	prepareXID	Prepare a distributed transaction.
**	commitXID	Commit a distributed transaction.
**	rollbackXID	Rollback a distributed transaction.
**	recoverXID	Retrieve list of distributed transaction IDs.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
*/

interface
EdbcXARM
{

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
*/

void registerXID( XAResource rsrc, EdbcXAXid xid ) throws XAException;


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
*/

void deregisterXID( XAResource rsrc, EdbcXAXid xid ) throws XAException;


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
*/

int prepareXID( EdbcXAXid xid ) throws XAException;


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
*/

void commitXID( EdbcXAXid xid, boolean onePhase ) throws XAException;


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
*/

void rollbackXID( EdbcXAXid xid ) throws XAException;


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
*/

Xid[] recoverXID() throws XAException;


} // interface EdbcXARM
