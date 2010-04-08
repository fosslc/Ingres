
/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcNlob.java
**
** Description:
**	Defines class which implements the JDBC NClob interface.
**
**	Class acts as a wrapper for NClob objects representing both
**	LOB Locators and cached LOBs.
**
**  Classes:
**
**	JdbcNlob
**
** History:
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

import	java.sql.NClob;
import	java.sql.Clob;
import  com.ingres.gcf.util.BufferedNlob;

/*
** Name: JdbcNlob
**
** Description:
**	JDBC driver class which implements the JDBC NClob interface.
**
** History:
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

public class
JdbcNlob
    extends JdbcClob
    implements NClob, DrvConst
{

/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
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
**      11-May-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( DrvTrace trace )
{
    this( DRV_DFLT_SEGSIZE, trace );
} // JdbcNlob

/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
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
**      11-May-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( int segSize, DrvTrace trace )
{
    super( new BufferedNlob( segSize ), segSize, trace, "Nlob[cache]" );
} // JdbcNlob

/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
**
** Input:
**	nlob	Underlying DBMS NClob.
**	trace	Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( DrvNlob nlob, DrvTrace trace )
{
    this( nlob, DRV_DFLT_SEGSIZE, trace );
} // JdbcNlob

/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
**
** Input:
**	clob	Underlying DBMS NClob.
**	trace	Tracing output.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( DrvClob clob, DrvTrace trace )
{
    this( clob, DRV_DFLT_SEGSIZE, trace );
} // JdbcNlob


/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
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
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( DrvNlob nlob, int segSize, DrvTrace trace )
{
    super( nlob, segSize, trace, "Nlob[LOC:" + nlob.toString() + "]" );
} // JdbcNlob

/*
** Name: JdbcNlob
**
** Description:
**	Class constructor for NCS (UCS2) LOB Locators.
**
** Input:
**	clob		Underlying DBMS NClob.
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
**      30-Mar-09 (rajus01) SIR 121238
**	    Created.
*/

// package access
JdbcNlob( DrvClob clob, int segSize, DrvTrace trace )
{
    super( clob, segSize, trace, "Clob[LOC:" + clob.toString() + "]" );
} // JdbcNlob

} // class JdbcNlob
