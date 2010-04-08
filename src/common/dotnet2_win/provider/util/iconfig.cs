/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: iconfig.cs
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
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: Config
	**
	** Description:
	**	Defines access to configuration information.  Configuration
	**	information is stored as a String and accessed via a key 
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
	**	get
	**
	** History:
	**	15-Jul-03 (gordy)
	**	    Created.
	*/

	/// <summary>
	/// Configuration interface for connection string parameters.
	/// </summary>
	public interface
		IConfig
	{


		/*
		** Name: Get
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

		/// <summary>
		/// get the value of the connection string parameters for a given key.
		/// </summary>
		/// <param name="key"></param>
		/// <returns></returns>
		String get( String key );

	} // Interface Config
}  // namespace
