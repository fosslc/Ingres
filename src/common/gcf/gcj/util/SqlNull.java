/*
** Copyright (c) 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlNull.java
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

public class
SqlNull
    extends SqlData
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
SqlNull()
{
    super( true );
} // SqlNull


} // class SqlNull
