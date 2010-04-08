/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqlnull.cs
	**
	** Description:
	**	Defines class which represents a generic SQL NULL value.
	**
	**  Classes:
	**
	**	SqlNull		A generic SQL NULL value.
	**
	** History:
	**	22-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: SqlNull
	**
	** Description:
	**	Class which represents an SQL generic NULL value.
	**
	**	Data values of this type are always NULL: a non-NULL value
	**	cannot be assigned and no converstions are supported.
	**
	**  Inherited Methods:
	**
	**	setNull			Set data value NULL.
	**	isNull			Data value is NULL?
	**
	** History:
	**	22-Sep-03 (gordy)
	**	    Created.
	*/

	internal class
		SqlNull : SqlData
	{


		/*
		** Name: SqlNull
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public
			SqlNull() : base( true )
		{
		} // SqlNull


	} // class SqlNull
}  // namespace
