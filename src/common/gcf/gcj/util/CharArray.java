/*
** Copyright (c) 2003 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: CharArray.java
**
** Description:
**	Defines interface for accessing a variable length character array.
**
**  Interfaces
**
**	CharArray
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added get() method to access specific array element.
*/


/*
** Name: CharArray
**
** Description:
**	Interface which provides access to an object as a character array.
**
**	Character arrays are variable length with a current length and
**	capacity.  Character arrays may also have an optional maximum
**	length (size limit).  
**
**	Initially, character arrays have zero length, an implementation 
**	determined capacity and no size limit.  The length of the array 
**	can grow until it reaches the size limit (if set).  Array capacity
**	is automatically extended to accomodate growth requests. A minimum
**	capacity may be set to minimize costly changes in the capacity of 
**	the array.
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
**	get		Copy characters out of the array.
**	put		Copy characters into the array.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added get() method to access specific array element.
*/

public interface
CharArray
    extends VarArray
{


/*
** Name: get
**
** Description:
**	Returns the character at a specific position in the 
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
**	char		Character or 0.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

char get( int position );


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
**	char[]	Copy of the current array.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

char[] get();


/*
** Name: get
**
** Description:
**	Copy characters out of the array.  Copying starts with the first
**	character of the array.  The number of characters copied is the 
**	minimum of the current array length and the length of the output 
**	array.
**
** Input:
**	None.
**
** Output:
**	buff	Character array to receive output.
**
** Returns:
**	int	Number of characters copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int get( char[] buff );


/*
** Name: get
**
** Description:
**	Copy a portion of the array.  Copying starts at the position
**	indicated.  The number of characters copied is the minimum of 
**	the length requested and the number of characters in the array 
**	starting at the requested position.  If position is greater than 
**	the current length, no characters are copied.  The output array 
**	must have sufficient space.  The actual number of characters 
**	copied is returned.
**
** Input:
**	position	Starting character to copy.
**	length		Number of characters to copy.
**	offset		Starting position in output array.
**
** Output:
**	buff		Character array to recieve output.
**
** Returns:
**	int		Number of characters copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int get( int position, int length, char[] buff, int offset );


/*
** Name: put
**
** Description:
**	Append a character value to the current array data.  The 
**	portion of the input which would cause the array to grow 
**	beyond the size limit (if set) is ignored.  The number of 
**	characters actually appended is returned.
**
** Input:
**	value	The character to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( char value );


/*
** Name: put
**
** Description:
**	Append a character array to the current array data.  The 
**	portion of the input which would cause the array to grow 
**	beyond the size limit (if set) is ignored.  The number of 
**	characters actually appended is returned.
**
** Input:
**	chars	The character array to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( char[] chars );


/*
** Name: put
**
** Description:
**	Append a portion of a character array to the current array 
**	data.  The portion of the input which would cause the array 
**	to grow beyond the size limit (if set) is ignored.  The 
**	number of characters actually appended is returned.
**
** Input:
**	chars	Array containing characters to be appended.
**	offset	Start of portion to append.
**	length	Number of bytes to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

int put( char[] chars, int offset, int length );


} // interface CharArray

