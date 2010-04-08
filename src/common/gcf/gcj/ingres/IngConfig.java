/*
** Copyright (c) 2003, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngConfig.java
**
** Description:
**	Defines class which establishes the Ingres JDBC Driver runtime
**	environment.
**
**  Classes
**
**	IngConfig
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
*/

import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.ConfigFile;
import	com.ingres.gcf.util.ConfigKey;
import	com.ingres.gcf.util.ConfigProp;
import	com.ingres.gcf.util.ConfigSys;


/*
** Name: IngConfig
**
** Description:
**	Class which establishes the Ingres JDBC Driver runtime environment.
**
**  Public Methods:
**
**	getConfig	Returns driver configuration info.
**	getMajorVers	Returns driver major version.
**	getMinorVers	Returns driver minor version.
**	setDriverVers	Set driver version info.
**
**  Private Data:
**
**	config		Ingres JDBC Driver configuration info.
**	majorVers	Driver major version.
**	minorVers	Driver minor version.
**	patchVers	Driver patch version.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**      14-Feb-08 (rajus01) SIR 119917
**          Added support for JDBC driver patch version for patch builds.
*/

class
IngConfig
    implements IngConst
{

    private static Config	config		= null;	// Ingres config info
    private static int		majorVers	= 0;	// Driver major version
    private static int		minorVers	= 0;	// Driver minor version
    private static int		patchVers	= 0;	// Driver patch version


/*
** Name: getConfig
**
** Description:
**	Returns the Ingres Driver configuration information.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Config	Driver config info.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Added synchronization to protect config as it changes during
**	    initialization.
*/

public static synchronized Config
getConfig()
{
    if ( config == null )
    {
	/*
	** Ingres JDBC Driver configuration information is formed
	** from a heirarchy of the following sources:
	**
	**	Default properties file (iijdbc.properties)
	**	Runtime properties file
	**	System properties
	**
	** Ingres configuration keys are prefixed with 'ingres.jdbc'.
	*/
	try { config = new ConfigFile( ING_PROP_FILE_NAME, config ); }
	catch( Exception ignore ) {}
	
	try
	{
	    /*
	    ** See if runtime properties file is defined in the
	    ** system properties: ingres.jdbc.property_file
	    */
	    String name = System.getProperty( ING_CONFIG_KEY + "." + 
					      ING_PROP_FILE_KEY );
	    if ( name != null )  config = new ConfigFile( name, config );
	}
	catch( Exception ignore ) {}
	
	config = new ConfigSys( config );		// System properties
	config = new ConfigKey( ING_CONFIG_KEY, config );    // Ingres prefix
    }
    
    return( config );
}


/*
** Name: getMajorVers
**
** Description:
**	Returns the driver's major version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Driver major version.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

public static int
getMajorVers()
{
    return( majorVers );
}


/*
** Name: getMinorVers
**
** Description:
**	Returns the driver's minor version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Driver minor version.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

public static int
getMinorVers()
{
    return( minorVers );
}

/*
** Name: getPatchVers
**
** Description:
**	Returns the driver's patch version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Driver patch version.
**
** History:
**	31-Jan-08 (rajus01)
**	    Created.
*/

public static int
getPatchVers()
{
    return( patchVers );
}

/*
** Name: setDriverVers
**
** Description:
**	Set the driver version information.
**
** Input:
**	majorVers	Driver major version.
**	minorVers	Driver minor version.
**	patchVers	Driver patch version.
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

public static void
setDriverVers( int majorVers, int minorVers, int patchVers )
{
    IngConfig.majorVers = majorVers;
    IngConfig.minorVers = minorVers;
    IngConfig.patchVers = patchVers;
    return;
}


/*
** Name: IngConfig
**
** Description:
**	Class constructor.  No instances are allowed.
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

private IngConfig() {}


} // class IngConfig
