/*
** Copyright (c) 2003, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ConfigKey.java
**
** Description:
**	Defines a class which implements the Config interface and
**	provides a key heirarchy.
**
**  Classes
**
**	ConfigKey
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
**	Class which implements a key heirarchy by prefixing keys
**	prior to searching nested info sets.
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
ConfigKey
    implements Config
{

    private	String		prefix = null;
    private	Config		config = null;
    
    
/*
** Name: ConfigKey
**
** Description:
**	Class constructor.  Defines the key heirarchy prefix and
**	nested configuration information set.  The nested set will
**	be searched with keys in the following form: <prefix>.<key>
**
** Input:
**	prefix	Heirarchy key prefix.
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
ConfigKey( String prefix, Config config )
    throws NullPointerException
{
    if ( prefix == null  ||  config == null )
	throw new NullPointerException();

    this.prefix = prefix;
    this.config = config;
} // ConfigKey


/*
** Name: get
**
** Description:
**	Prefixes the provided key with this heirarchy prefix and
**	searches the nested configuration information set.
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
    return( config.get( prefix + "." + key ) );
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
    return( config.getKey( prefix ) );
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
    return( config.getKey( prefix + "." + key ) );
} // getKey


} // Class ConfigKey
