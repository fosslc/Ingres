/*
** Copyright (c) 2003, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: Config.java
**
** Description:
**	Defines an interface which provides configuration info access.
**
**  Interfaces
**
**	Config
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added ability to determine fully qualified keys when
**	    dealing with a nested configuration stack.
*/


/*
** Name: Config
**
** Description:
**	Defines access to configuration information.  Configuration
**	information is stored as a Java String and accessed via a key 
**	String.  
**
**	It is expected that implementations of this interface will 
**	utilize Hashtable, Properties or some other suitable object
**	to store configuration information and provide a constructor
**	which allows initialization of the info set.  Implementations
**	should also allow for stacking or nesting of Config objects
**	to form a heirarchy of info sets.
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

public interface
Config
{

    
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

String get( String key );


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

String getKey();


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

String getKey( String key );


} // Interface Config
