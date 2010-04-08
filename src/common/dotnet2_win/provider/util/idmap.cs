/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	using System;
	
	/*
	** Name: idmap.cs
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
	**	24-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
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
	**	24-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	*/
	
	internal class IdMap
	{
		private int id; // numeric ID.
		private String name; // string representation.
		
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
		**	    Created.*/
		
		public IdMap(int id, String name)
		{
			this.id = id;
			this.name = name;
		}
		// IdMap


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
		**
		** History:
		**	02-Nov-09 (thoda04)
		**	    Made public.
		*/

		public bool equals(int id)
		{
			return (this.id == id);
		}
		// equals
		
		
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
		**	    Created.*/
		
		public override String ToString()
		{
			return (name);
		}
		// toString
		
		
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
		**	    Created.*/
		
		public static String get(int id, IdMap[] idMap)
		{
			 for (int i = 0; i < idMap.Length; i++)
				if (idMap[i].equals(id))
				{
					return (idMap[i].ToString());
				}
			return (null);
		}
		// get
		
		
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
		**	    Created.*/
		
		public static String map(int id, IdMap[] idMap)
		{
			String name = get(id, idMap);
			return (name != null?name:"[" + id + "]");
		}
		// map
	}
	// class IdMap
}