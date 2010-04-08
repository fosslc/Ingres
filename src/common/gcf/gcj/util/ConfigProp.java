/*
** Copyright (c) 2003, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ConfigProp.java
**
** Description:
**	Defines a class which implements the Config interface and
**	provides access to a Properties object.
**
**  Classes
**
**	ConfigProp
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added ability to determine fully qualified keys when
**	    dealing with a nested configuration stack.
*/

import	java.util.Properties;


/*
** Name: ConfigProp
**
** Description:
**	Class which provides access to config items in a Properties object.
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
ConfigProp
    implements Config
{

    private	Properties	properties = null;
    private	Config		config = null;
    
    
/*
** Name: ConfigProp
**
** Description:
**	Class constructor.  Defines a property set to search for config
**	information.
**
** Input:
**	properties	Property set.
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
ConfigProp( Properties properties )
    throws NullPointerException
{
    this( properties, (Config)null );
} // ConfigProp


/*
** Name: ConfigProp
**
** Description:
**	Class constructor.  Defines a property set to search for config
**	information and a nested info set to be searched if config items
**	are not defined in the property set.  Providing a null object for  
**	the nested info set is the same as using the constructor with a 
**	single parameter.
**
** Input:
**	properties	Property set.
**	config		Nested information set, may be null.
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
ConfigProp( Properties properties, Config config )
    throws NullPointerException
{
    if ( properties == null )  throw new NullPointerException();
    this.properties = properties;
    this.config = config;
} // ConfigProp


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
    String item = properties.getProperty( key );
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


} // Class ConfigProp
