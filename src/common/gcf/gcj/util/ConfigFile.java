/*
** Copyright (c) 2003, 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ConfigFile.java
**
** Description:
**	Defines a class which implements the Config interface and
**	provides access to a properties file.
**
**  Classes
**
**	ConfigFile
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added ability to determine fully qualified keys when
**	    dealing with a nested configuration stack.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
*/

import	java.io.File;
import	java.io.FileInputStream;
import	java.io.InputStream;
import	java.io.IOException;
import	java.io.FileNotFoundException;
import	java.net.URL;
import	java.util.Properties;


/*
** Name: ConfigFile
**
** Description:
**	Class which provides access to config items in a properties file.
**
**  Public Methods:
**
**	get		Returns a config item value.
**	getKey		Returns a fully qualified item key.
**
**  Private Methods:
**
**	initialize	Initialize this Config object.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Added getKey().
*/

public class
ConfigFile
    implements Config
{

    private	Properties	properties = null;
    private	Config		config = null;
    
    
/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from an input stream.
**
** Input:
**	is	Input stream.
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
ConfigFile( InputStream is )
    throws IOException, IllegalArgumentException, NullPointerException
{
    this( is, (Config)null );
} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from an input stream and 
**	defines a nested info set to be searched if config items are not 
**	found in the property set.  Providing a null object for the nested 
**	info set is the same as using the constructor with a single parameter.
**
** Input:
**	is		Input stream.
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
ConfigFile( InputStream is, Config config )
    throws IOException, IllegalArgumentException, NullPointerException
{
    if ( is == null )  throw new NullPointerException();
    initialize( is, config );
} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a property file.
**
** Input:
**	file	Property File.
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
ConfigFile( File file )
    throws IOException, IllegalArgumentException, NullPointerException,
	   FileNotFoundException, SecurityException
{
    this( file, (Config)null );
} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a property file and 
**	defines a nested info set to be searched if config items are not 
**	found in the property set.  Providing a null object for the nested 
**	info set is the same as using the constructor with a single parameter.
**
** Input:
**	file		Property File.
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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Close opened stream.
*/

public
ConfigFile( File file, Config config )
    throws IOException, IllegalArgumentException, NullPointerException,
	   FileNotFoundException, SecurityException
{
    if ( file == null )  throw new NullPointerException();
    InputStream is = new FileInputStream( file );

    try { initialize( is, config ); }
    finally { is.close(); }

} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a property file
**	identified by a URL.
**
** Input:
**	url	URL.
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
ConfigFile( URL url )
    throws IOException, IllegalArgumentException, NullPointerException
{
    this( url, (Config)null );
} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a property file 
**	identified by a URL and defines a nested info set to be searched 
**	if config items are not found in the property set.  Providing a 
**	null object for the nested info set is the same as using the 
**	constructor with a single parameter.
**
** Input:
**	url		URL.
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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Close opened stream.
*/

public
ConfigFile( URL url, Config config )
    throws IOException, IllegalArgumentException, NullPointerException
{
    if ( url == null )  throw new NullPointerException();
    InputStream is = url.openStream();

    try { initialize( is, config ); }
    finally { is.close(); }

} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a named property file.
**
**	Access to the named file is first made as if it were a local disk
**	file with sufficient path information to locate the file.  If the
**	file cannot be accessed, then an attempt is made to access the
**	file as a Java resource (file must exist in the class load path).
**
** Input:
**	name	File name.
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
ConfigFile( String name )
    throws IOException, IllegalArgumentException, NullPointerException,
	   FileNotFoundException, SecurityException
{
    this( name, (Config)null );
} // ConfigFile


/*
** Name: ConfigFile
**
** Description:
**	Class constructor.  Loads a property set from a named property file 
**	and defines a nested info set to be searched if config items are not 
**	found in the property set.  Providing a null object for the nested 
**	info set is the same as using the constructor with a single parameter.
**
** Input:
**	name		File name.
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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Close opened stream.
*/

public
ConfigFile( String name, Config config )
    throws IOException, IllegalArgumentException, NullPointerException,
	   FileNotFoundException, SecurityException
{
    InputStream is = null;
    
    if ( name == null )  throw new NullPointerException();
    
    /*
    ** See if file is accessible directly from the name provided.
    */
    try { is = new FileInputStream( name ); }
    catch( Exception ignore ) {}
    
    try 
    {
	if ( is == null )
	{
	    /*
	    ** See if file can be accesed as a Java resource.
	    */
	    URL url = ConfigFile.class.getClassLoader().getResource( name );
	
	    if ( url != null )
		is = url.openStream();
	    else
		throw new FileNotFoundException();
	}
    
	initialize( is, config );
    }
    finally
    {
	if ( is != null )  is.close();
    }

} // ConfigFile


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


/*
** Name: initialize
**
** Description:
**	Initialize this Config object from an input stream (to a properties
**	file) and a nested (possibly null) configuration set.
**
** Input:
**	is	Input stream.
**	config	Nested configuration set, may be null.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

private void
initialize( InputStream is, Config config )
    throws IOException, IllegalArgumentException
{
    this.config = config;
    properties = new Properties();
    properties.load( is );
    return;
} // initialize


} // Class ConfigFile
