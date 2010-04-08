/*
** Copyright (c) 2003 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: ByteArray.java
**
** Description:
**	Defines interface for accessing a variable length byte array.
**
**  Interfaces
**
**	ByteArray
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added get() method to access specific array element.
*/


/*
** Name: ByteArray
**
** Description:
**	Interface which provides access to an object as a byte array.
**
**	Byte arrays are variable length with a current length and
**	capacity.  Byte arrays may also have an optional maximum
**	length (size limit).  
**
**	Initially, byte arrays have zero length, an implementation 
**	determined capacity and no size limit.  The length of the
**	array can grow until it reaches the size limit (if set).  
**	Array capacity is automatically extended to accomodate growth 
**	requests.  A mininum capacity may be set to minimize costly 
**	changes in the capacity of the array.
**
**  Inherited Methods:
**
**	capacity	Determine capacity of the array.
**	ensureCapacity	Set minimum capacity of the array.
**	limit		Set or determine maximum capacity of the array.
**	length		Determine the current length of the array.
**	clear		Set array length to zero.
**
**  Public Methods:
**
**	get		Copy bytes out of the array.
**	put		Copy bytes into the array.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added get() method to access specific array element.
*/

public interface
ByteArray
    extends VarArray
{


/*
** Name: get
**
** Description:
**	Returns the byte value at a specific position in the
**	array.  If position is beyond the current array length, 
**	0 is returned.
**
** Input:
**	position	Array position.
**
** Output:
**	None.
**
** Returns:
**	byte		Byte value or 0.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

byte get( int position );


/*
** Name: get
**
** Description:
**	Returns a copy of the array.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Copy of the current array.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

byte[] get();


/*
** Name: get
**
** Description:
**	Copy bytes out of the array.  Copying starts with the first
**	byte of the array.  The number of bytes copied is the minimum
**	of the current array length and the length of the output array.
**
** Input:
**	None.
**
** Output:
**	buff	Byte array to receive output.
**
** Returns:
**	int	Number of bytes copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int get( byte[] buff );


/*
** Name: get
**
** Description:
**	Copy a portion of the array.  Copying starts at the position
**	indicated.  The number of bytes copied is the minimum of the
**	length requested and the number of bytes in the array starting
**	at the requested position.  If position is greater than the
**	current length, no bytes are copied.  The output array must
**	have sufficient space.  The actual number of bytes copied is
**	returned.
**
** Input:
**	position	Starting byte to copy.
**	length		Number of bytes to copy.
**	offset		Starting position in output array.
**
** Output:
**	buff		Byte array to recieve output.
**
** Returns:
**	int		Number of bytes copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int get( int position, int length, byte[] buff, int offset );


/*
** Name: put
**
** Description:
**	Append a byte value to the current array data.  The portion
**	of the input which would cause the array to grow beyond the
**	size limit (if set) is ignored.  The number of bytes actually 
**	appended is returned.
**
** Input:
**	value	The byte to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( byte value );


/*
** Name: put
**
** Description:
**	Append a byte array to the current array data.  The portion
**	of the input which would cause the array to grow beyond the
**	size limit (if set) is ignored.  The number of bytes actually 
**	appended is returned.
**
** Input:
**	bytes	The byte array to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( byte[] bytes );


/*
** Name: put
**
** Description:
**	Append a portion of a byte array to the current array data.  
**	The portion of the input which would cause the array to grow 
**	beyond the size limit (if set) is ignored.  The number of 
**	bytes actually appended is returned.
**
** Input:
**	bytes	Array containing bytes to be appended.
**	offset	Start of portion to append.
**	length	Number of bytes to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( byte[] bytes, int offset, int length );


} // interface ByteArray

