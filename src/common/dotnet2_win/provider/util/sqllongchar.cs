/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqllongchar.cs
	**
	** Description:
	**	Defines class which represents an SQL LongVarchar value.
	**
	**  Classes:
	**
	**	SqlLongChar	An SQL LongVarchar value.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Dec-03 (gordy, ported by thoda04 1-Oct-06)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	*/


	/*
	** Name: SqlLongChar
	**
	** Description:
	**	Class which represents an SQL LongVarchar value.  SQL 
	**	LongVarchar values are potentially large variable length 
	**	streams.
	**
	**	Supports conversion to String, Binary, Object.  
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**	This class supports both byte and character streams by
	**	extending SqlLongNChar class and requiring a character-
	**	set which defines the conversion between bytes strea and 
	**	character stream and implementing the cnvtIS2Rdr method 
	**	to wrap the InputStream with a Reader to perform the 
	**	charactger-set conversion.
	**
	**  Inherited Methods:
	**
	**	setNull			Set a NULL data value.
	**	toString		String representation of data value.
	**	closeStream		Close the current stream.
	**
	**	setBoolean		Data value accessor assignment methods
	**	setByte
	**	setShort
	**	setInt
	**	setLong
	**	setFloat
	**	setDouble
	**	setBigDecimal
	**	setString
	**	setDate
	**	setTime
	**	setTimestamp
	**	setCharacterStream
	**
	**	getBoolean		Data value accessor retrieval methods
	**	getByte
	**	getShort
	**	getInt
	**	getLong
	**	getFloat
	**	getDouble
	**	getBigDecimal
	**	getString
	**	getDate
	**	getTime
	**	getTimestamp
	**	getAsciiStream
	**	getUnicodeStream
	**	getCharacterStream
	**	getObject
	**
	**  Public Methods:
	**
	**	set			Assign a new non-NULL data value.
	**	get			Retrieve current data value.
	**
	**  Protected Mehtods:
	**
	**	cnvtIS2Rdr		Convert InputStream to Reader.
	**
	**  Private Data:
	**
	**	charSet			Character-set.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	*/

	internal class
		SqlLongChar : SqlLongNChar
	{

		private CharSet		charSet = null;


		/*
		** Name: SqlLongChar
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**
		** Input:
		**	charSet		Character-set of byte stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public
			SqlLongChar( CharSet charSet ) : base( )
		{
			this.charSet = charSet;
		} // SqlLongChar


		/*
		** Name: SqlLongChar
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**	Defines a SqlStream event listener for stream 
		**	closure event notification.
		**
		** Input:
		**	charSet		Character-set of byte stream.
		**	listener	Stream listener.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		internal
			SqlLongChar( CharSet charSet, IStreamListener listener ) : base( listener )
		{
			this.charSet = charSet;
		} // SqlLongChar


		/*
		** Name: set
		**
		** Description:
		**	Assign a new non-NULL data value.
		**
		** Input:
		**	stream		The new data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			set( InputStream stream )
		{
			setStream(stream);
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlChar data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlChar data value to copy.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		set(SqlChar data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The character data is stored in a byte array in
				** the host character-set.  A simple binary stream
				** will produce the desired output.  Note that we
				** need to follow the SqlChar convention and extend
				** the data to the optional limit.
				*/
				data.extend();
				setStream(getBinary(data.value, 0, data.length));
			}
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlVarChar data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlVarChar data value to copy.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		set(SqlVarChar data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The character data is stored in a byte array in
				** the host character-set.  A simple binary stream
				** will produce the desired output.
				*/
				setStream(getBinary(data.value, 0, data.length));
			}
			return;
		} // set


		/*
		** Name: get
		**
		** Description:
		**	Return the current data value.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Current value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override Reader
		get()
		{
			Object stream = getStream();
//			Reader rdr;

			if (stream == null)
				return (null);
			else
				if (stream is InputStream)
					return (cnvtIS2Rdr((InputStream)stream));
				else
					if (stream is Reader)
						return ((Reader)stream);
					else
						throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Write the current data value to an OutputStream.
		**	The current data value is consumed.  The Output-
		**	Stream is not closed.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	os	OutputStream to receive data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		get(OutputStream os)
		{
			Object stream = getStream();

			if (stream != null)
				if (stream is InputStream)
					copyIs2Os((InputStream)stream, os);
				else if (stream is Reader)
				{
					OutputStreamWriter wtr;

					try { wtr = charSet.getOSW(os); }
					catch (Exception /* ex */)	// Should not happen!
					{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE); }

					copyRdr2Wtr((Reader)stream, wtr);
				}
				else
					throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);
			return;
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Write the current data value to a Writer stream.
		**	The current data value is consumed.  The Writer
		**	stream is not closed but will be flushed.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	wtr	Writer to receive data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override void
		get(Writer wtr)
		{
			Object stream = getStream();

			if (stream != null)
			{
				if (stream is InputStream)
					copyRdr2Wtr(cnvtIS2Rdr((InputStream)stream), wtr);
				else
					if (stream is Reader)
						copyRdr2Wtr((Reader)stream, wtr);
					else
						throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);
			}
			return;
		} // get


		/*
		** Name: cnvtIS2Rdr
		**
		** Description:
		**	Converts the byte string input stream into a Reader
		**	stream using the associated character-set.
		**
		** Input:
		**	stream	    Input stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader		Converted Reader stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		*/

		protected override Reader
			cnvtIS2Rdr( InputStream stream )
		{
			try 
			{ 
				return( charSet.getISR( stream ) ); 
			}
			catch( Exception ex)			// Should not happen!
			{
				throw SqlEx.get( ERR_GC401E_CHAR_ENCODE, ex );
			}
		} // cnvtIS2Rdr


	} // class SqlLongChar
}  // namespace
