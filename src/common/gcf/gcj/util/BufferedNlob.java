
/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: BufferedNlob.java
**
** Description:
**	Defines class which implements the JDBC NClob interface
**	as a wrapper for a cached NCLOB value.
**
**  Classes:
**
**	BufferedNlob		NClob wrapper for CharBuffer
**
** History:
**	 9-Apr-09 (rajus01)
**	    Created.
*/

import	java.io.Reader;
import	java.sql.NClob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: BufferedNlob
**
** Description:
**	Class which implements the JDBC NClob interface as
**	a wrapper for a cached NCLOB value.
**
**	The NCLOB data is stored in a CharBuffer.
**
** History:
**	 9-Apr-09 (rajus01)
**	    Created.
*/

public class
BufferedNlob
    extends BufferedClob
    implements NClob, GcfErr
{

/*
** Name: BufferedNlob
**
** Description:
**	Class constructor.  Constructs an empty NClob
**	with default segment size.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Apr-09 (rajus01)
**	    Created.
*/

public
BufferedNlob()
{
   super();
} // BufferedNlob


/*
** Name: BufferedNlob
**
** Description:
**	Class constructor.  Constructs an empty NClob
**	with requested segment size.
**
** Input:
**	size	Block size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Apr-09 (rajus01)
**	    Created.
*/

public
BufferedNlob( int size )
{
    super( size );
} // BufferedNlob


/*
** Name: BufferedNlob
**
** Description:
**	Class constructor.  Constructs an empty NClob
**	with default segment size and fills with data
**	read from a character stream.  The character 
**	stream is closed.
**
** Input:
**	stream	Character stream.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Apr-09 (gordy)
**	    Created.
*/

public
BufferedNlob( Reader stream )
{
    super( stream );
} // BufferedNlob


/*
** Name: BufferedNlob
**
** Description:
**	Class constructor.  Constructs an empty NClob
**	with requested segment size and fills with data
**	read from a character stream.  The character 
**	stream is closed.
**
** Input:
**	size	Block size.
**	stream	Character stream.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Apr-09 (gordy)
**	    Created.
*/

public
BufferedNlob( int size, Reader stream )
{
    super( size, stream );
} // BufferedNlob

    
/*
** Name: BufferedNlob
**
** Description:
**	Class constructor.  Constructs a NClob as a
**	wrapper for an existing char buffer.
**
** Input:
**	buff	Character buffer.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	9-Apr-09 (rajus01)
**	    Created.
*/

public
BufferedNlob( CharBuffer buff )
{
    super(buff);
} // BufferedNlob

} // class BufferedNlob
