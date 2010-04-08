/*
** Copyright (c) 2003, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ConfigSys.java
**
** Description:
**	Defines a class which implements the Config interface and
**	provides access to system configuration properties.
**
**  Classes
**
**	ConfigSys
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added ability to determine fully qualified keys when
**	    dealing with a nested configuration stack.
*/


/*
** Name: ConfigSys
**
** Description:
**	Class which provides access to system configuration properties.
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
ConfigSys
    implements Config
{

    private	Config		config = null;
    
    
/*
** Name: ConfigSys
**
** Description:
**	Class constructor.  Basic system configuration property
**	access with no nested info sets.
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
ConfigSys()
{
} // ConfigSys


/*
** Name: ConfigSys
**
** Description:
**	Class constructor.  Defines a nested info set to be searched
**	if config item is not in system properties.  Providing a null
**	object for the info set is the same as using the constructor 
**	with no parameters.
**
** Input:
**	config	Nested information set, may be null.
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
ConfigSys( Config config )
{
    this.config = config;
} // ConfigSys


/*
** Name: get
**
** Description:
**	Search for and return a configuration item for a given key.
**	If the configuration item is not defined in this set, any
**	nested set is searched as well.
**
** Input:
**	key	Identifies a configuration item.
**
** Output:
**	None.
**
** Returns:
**	String	The configuration item or null if not defined.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

public String 
get( String key )
{
    String item = null;
    
    try { item = System.getProperty( key ); }
    catch( Exception ignore ) {}
    
    if ( item == null  &&  config != null )  item = config.get( key );
    return( item );
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
    return( (config != null) ? config.getKey() : "" );
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
    return( (config != null) ? config.getKey( key ) : key );
} // getKey


} // Class ConfigSys
