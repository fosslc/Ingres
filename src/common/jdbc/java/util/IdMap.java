/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: IdMap.java
**
** Description:
**	Defines a class which provides a mapping between numeric
**	constants and their alpha-numeric identifiers.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Moved to util package.
*/


/*
** Name: IdMap
**
** Description:
**	Provides for mapping of numeric ID's into a their string
**	representations.  This class is designed to permit easy
**	instantiation in static declarations (such as interfaces)
**	as an array.  Static functions then provide access to the
**	mappings in the array.
**
**  Public Methods:
**
**	toString    Returns the string representation of an ID.
**	get	    Searches array, returns the string representation
**	map	    Searches array, returns a string representation.
**
**  Private Data:
**
**	id	    The numeric ID.
**	name	    The string representation.
**
**  Private Methods:
**
**	equals	    Does an IdMap map a particular numeric ID.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Removed tracing dependencies by making data private,
**	    adding access and static mapping functions.
*/

public class
IdMap
{
    private int	    id;		// numeric ID.
    private String  name;	// string representation.

/*
** Name: IdMap
**
** Description:
**	Class constructor.
**
** Input:
**	id	The numeric ID.
**	name	The string representation.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
*/

public
IdMap( int id, String name )
{
    this.id = id;
    this.name = name;
} // IdMap


/*
** Name: equals
**
** Description:
**	Returns indication that this IdMap maps the provided 
**	numeric ID.
**
** Input:
**	id	Numeric ID.
**
** Output:
**	None.
**
** Returns:
**	boolean	True if this IdMap represents id.
*/

private boolean
equals( int id )
{
    return( this.id == id );
} // equals


/*
** Name: toString
**
** Description:
**	Returns the string representation of the mapped ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	String representaiton of id.
**
** History:
**	18-Apr-00 (gordy)
**	    Created.
*/

public String
toString()
{
    return( name );
} // toString


/*
** Name: get
**
** Description:
**	Searches an IdMap array for the IdMap which matches the
**	numeric ID provided and returns the string ID.  If no
**	match is found, NULL is returned.
**
** Input:
**	id	Numeric ID.
**	map	IdMap array.
**
** Output:
**	None.
**
** Returns:
**	String	String representation of id, or NULL.
**
** History:
**	18-Apr-00 (gordy)
**	    Created.
*/

public static String
get( int id, IdMap map[] )
{
    for( int i = 0; i < map.length; i++ )
	if ( map[i].equals( id ) )  return( map[i].toString() );
    return( null );
} // get

/*
 ** Name: getId
 **
 ** Description:
 **      Searches an IdMap array for the IdMap which matches the
 **      numeric ID provided and returns the string ID.  If no
 **      match is found, NULL is returned.
 **
 ** Input:
 **      String  String representation of id, or NULL.
 **      map     IdMap array.
 **
 ** Output:
 **      None.
 **
 ** Returns:
 **      id      Numeric ID
 **
 ** History:
 **      29-Jan-04 (fei wei) Bug # 111723; Startrak # EDJDBC94
 **          Created.
 */

public static int
getId( String s, IdMap map[] )
 {
     for( int i = 0; i < map.length; i++ )
         if ( s.trim().equals(map[i].name))
              return( map[i].id );
     return( 0 );
 } // getId


/*
** Name: map
**
** Description:
**	Searches an IdMap array for the IdMap which matches the
**	numeric ID provided and returns the string ID.  If no
**	match is found, the numeric ID is returned as a string.
**
** Input:
**	id	Numeric ID.
**	map	IdMap array.
**
** Output:
**	None.
**
** Returns:
**	String	String representation of id.
**
** History:
**	18-Apr-00 (gordy)
**	    Created.
*/

public static String
map( int id, IdMap map[] )
{
    String name = get( id, map );
    return( name != null ? name : "[" + id + "]" );
} // map

} // class IdMap
