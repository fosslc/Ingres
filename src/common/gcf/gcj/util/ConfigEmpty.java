/*
** Copyright (c) 2003, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ConfigEmpty.java
**
** Description:
**	Defines a class which implements the Config interface without
**	any associated properties.  May be used as default base when
**	no other Config object is available.
**
**  Classes
**
**	ConfigEmpty
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added ability to determine fully qualified keys when
**	    dealing with a nested configuration stack.
*/


/*
** Name: ConfigKey
**
** Description:
**	Class which provides a default base config set with no properties.
**
**  Public Methods:
**
**	get		Returns a config item value.
**	getKey		Returns a fully qualified item key.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added getKey().
*/

public class
ConfigEmpty
    implements Config
{

/*
** Name: ConfigEmpty
**
** Description:
**	Class constructor.  
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
**	15-Jul-03 (gordy)
**	    Created.
*/

public
ConfigEmpty()
{
} // ConfigEmpty


/*
** Name: get
**
** Description:
**	Simply returns null to indicate no associate properties.
**
** Input:
**	key	Identifies a configuration item.
**
** Output:
**	None.
**
** Returns:
**	String	Always returns null.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

public String 
get( String key )
{
    return( null );
} // get


/*
** Name: getKey
**
** Description:
**	Returns the key prefix used to search for config items.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Key prefix.
**
** History:
**	 3-Jul-06 (gordy)
**	    Created.
*/

public String
getKey()
{
    return( "" );
} // getKey


/*
** Name: getKey
**
** Description:
**	Returns the fully qualified key used to search for config items.
**
** Input:
**	key	Configuration item ID.
**
** Output:
**	None.
**
** Returns:
**	String	Fully qualified configuration key.
**
** History:
**	 3-Jul-06 (gordy)
**	    Created.
*/

public String 
getKey( String key )
{
    return( key );
} // getKey


} // Class ConfigEmpty
