/*
** Copyright (c) 2002, 2007 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	/*
	** Name: charset.cs
	**
	** Description:
	**
	**	Defines a class which provides mapping and conversion between
	**	Ingres installation character sets, characeter encodings
	**	and Unicode.
	**
	** History:
	**	21-Apr-00 (gordy)
	**	    Created.
	**	16-Aug-02 (gordy)
	**	    Added mappings for new character sets.
	**	17-Aug-02 (thoda04)
	**	    Ported to .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    Localized conversion in this class.
	**	 26-Jan-03 (weife01) Bug 109535
	**	    Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
	**	25-Feb-03 (gordy)
	**	    Added methods to handle character arrays in addition to strings.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 5-Mar-07 (thoda04)
	**	    Corrected typos in csInfo table.
	*/

	using System;

	/*
	** Name: CharSet
	**
	** Description:
	**	Maps Ingres character sets to .NET character encodings
	**	and provides conversion between Unicode and byte encoded
	**	strings.
	**
	**  Public Methods:
	**
	**	getCharSet	Factory for CharSet mappings.
	**	getByteLength	Returns length of converted string.
	**	getBytes  	Convert Unicode to bytes.
	**	getString 	Convert bytes to Unicode.
	**	getISR    	Get InputStreamReader for encoding.
	**	getOSW    	Get OutputStreamWriter for encoding.
	**
	**  Private Data:
	**
	**	enc       	Encoding for CharSet instance.
	**	multi_byte	Is encoding multi-byte.
	**	csMap     	Maps charsets to encodings.
	**	csInfo    	Charset/encoding mappings.
	**
	**  Private Methods:
	**
	**	getEncoding	Get the .NET encoding for a given character set.
	**
	** History:
	**	21-Apr-00 (gordy)
	**	    Extracted from class AdvanObject.
	**	16-Aug-02 (gordy)
	**	    Added mappings for new character sets.
	**	17-Aug-02 (thoda04)
	**	    Ported to .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    CharSet object now supports conversion for selected encoding.
	**	 26-Jan-03 (weife01) Bug 109535
	**	    Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
	*/

	internal class CharSet
	{
		private System.Text.Encoding encoding;
		private bool multi_byte; // Is encoding multi-byte?
		private byte space = 0;  // Space character in char-set
		private string ingresName = "";
		private string dotnetName = "";
		
		
		/*
		** Ingres character set to .NET character encoding mappings.
		** Each mapping may have an associated CharSet instance which
		** is created by the getCharSet() factory method.  The multi-
		** byte indication permits optimization in getByteLength().
		*/
		private static readonly System.Collections.Hashtable csMap =
			new System.Collections.Hashtable();
		
		private static Object[][] csInfo =
		{
			//           Ingres charset    .NET Encode   MBCS?  CS    CSid
			new Object[]{"ISO88591",       "iso-8859-1", false, null, 0x0001},
			new Object[]{"ISO88592",       "iso-8859-2", false, null, 0x0002},
			new Object[]{"ISO88595",       "iso-8859-5", false, null, 0x0005},
			new Object[]{"ISO88599",       "iso-8859-9", false, null, 0x0009},
			new Object[]{"IS885915",       "iso-8859-15",false, null, 0x000F},
			new Object[]{"IBMPC_ASCII_INT","cp850",      false, null, 0x0010},
			new Object[]{"IBMPC_ASCII",    "cp850",      false, null, 0x0010},
			new Object[]{"IBMPC437",       "cp437",      false, null, 0x0011},
			new Object[]{"ELOT437",        "cp737",      false, null, 0x0012},
			new Object[]{"SLAV852",        "cp852",      false, null, 0x0013},
			new Object[]{"IBMPC850",       "cp850",      false, null, 0x0014},
			new Object[]{"CW",             "cp1251",     false, null, 0x0015},
			new Object[]{"ALT",            "cp855",      false, null, 0x0016},
			new Object[]{"PC857",          "cp857",      false, null, 0x0017},
			new Object[]{"WIN1250",        "cp1250",     false, null, 0x0018},
			new Object[]{"KOI8",           "koi8-r",     false, null, 0x0019},
			new Object[]{"IBMPC866",       "cp866",      false, null, 0x001B},
			new Object[]{"WIN1252",        "windows-1252",false,null, 0x001C},
			new Object[]{"ASCII",          "ascii",      false, null, 0x0020},
			new Object[]{"DECMULTI",       "iso-8859-1", false, null, 0x0030},
			new Object[]{"HEBREW",         "iso-8859-8", false, null, 0x0031},
			new Object[]{"THAI",           "cp874",      false, null, 0x0032},
			new Object[]{"GREEK",          "cp875",      false, null, 0x0033},
			new Object[]{"HPROMAN8",       "iso-8859-1", false, null, 0x0040},
			new Object[]{"ARABIC",         "iso-8859-6", false, null, 0x0050},
			new Object[]{"WHEBREW",        "cp1255",     false, null, 0x0060},
			new Object[]{"PCHEBREW",       "cp862",      false, null, 0x0061},
			new Object[]{"WARABIC",        "cp1256",     false, null, 0x0062},
			new Object[]{"DOSASMO",        "cp864",      false, null, 0x0063},
			new Object[]{"WTHAI",          "cp874",      false, null, 0x0064},
			new Object[]{"EBCDIC_C",       "cp500",      false, null, 0x0100},
			new Object[]{"EBCDIC_ICL",     "cp500",      false, null, 0x0101},
			new Object[]{"EBCDIC_USA",     "cp037",      false, null, 0x0102},
			new Object[]{"EBCDIC",         "cp500",      false, null, 0x0102},
			new Object[]{"CHINESES",       "gb18030",    true,  null, 0x0200},
			new Object[]{"KOREAN",         "cp949",      true,  null, 0x0201},
			new Object[]{"KANJIEUC",       "euc-jp",     true,  null, 0x0202},
			new Object[]{"SHIFTJIS",       "shift_jis",  true,  null, 0x0203},
			new Object[]{"CHTBIG5",        "big5",       true,  null, 0x0205},
			new Object[]{"CHTEUC",         "EUC-TW",     true,  null, 0x0206},
			new Object[]{"CHTHP",          "EUC-TW",     true,  null, 0x0207},
			new Object[]{"CSGBK",          "GBK",        true,  null, 0x0208},
			new Object[]{"CSGB2312",       "gb2312",     true,  null, 0x0209},
			new Object[]{"UTF8",           "utf-8",      true,  null, 0x0311}};
		
		
		/*
		** Name: <class initializer>
		**
		** Description:
		**	Complete the initialization of static fields.
		**
		** History:
		**	21-Apr-00 (gordy)
		**	    Extracted from AdvanObject.
		**	 6-Sep-02 (gordy)
		**	    Moved encoding determination to checkEncoding(), hash table
		**	    value is now array index for character set mapping.
		*/
		
		
		static CharSet()
		{
			/*
			** Load the char set/encoding mappings of name to array index.
			*/
			for (int i = 0; i < csInfo.Length; i++)
			{
				csMap[csInfo[i][0]] = i;  // e.g. csMap["IS885915"] = 4;
				//string charName     = (String)csInfo[i][0];
				//string encodingName = (String)csInfo[i][1];
				//System.Text.Encoding encoding = getEncoding(encodingName);
				//if (encoding == null)
				//    Console.WriteLine(charName + "\t" + encodingName);
			}

		} // static


		/*
		** Name: CharSet
		**
		** Description:
		**	Class constructor for default encoding.
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
		**	26-Dec-02 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Since default encoding is implicitly accessed rather than
		**	    explicitly, assume access will be successful.  Drop the
		**	    encoding test and throw of exception.
		*/

		public
		CharSet()
		{
			this.encoding = null;		// Default encoding
			this.multi_byte = true;	// TODO: Is there a way to tell?
			return;
		} // CharSet


		/*
		** Name: CharSet
		**
		** Description:
		**	Class constructor.  Local instantiation only.
		**
		** Input:
		**	enc	    Encoding.
		**	multi_byte  Is encoding multi-byte.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	21-Apr-00 (gordy)
		**	    Created.
		**	17-Aug-02 (thoda04)
		**	    Ported to .NET Provider.
		**	 6-Sep-02 (gordy)
		**	    Added instance parameters.
		*/
		
		public CharSet(
			System.Text.Encoding encoding,
			bool multi_byte,
			string ingresName,
			string dotnetName)
		{
			this.encoding   = encoding;
			this.multi_byte = multi_byte;
			this.ingresName = ingresName;
			this.dotnetName = dotnetName;
		}  // CharSet


		/*
		** Name: getCharSet
		**
		** Description:
		**	Factory method for CharSet objects.  Determines mapping
		**	of character sets and encodings.  Maintains single CharSet
		**	per mapping.
		**
		** Input:
		**	cs_id	Character set ID.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	CharSet	    CharSet instance for character set.
		**
		** History:
		**	26-Dec-02 (gordy)
		**	    Created.
		*/

		public static CharSet getCharSet( int cs_id )
		{
			for( int i = 0; i < csInfo.Length; i++ )
				if ( (int)(csInfo[i][4]) == cs_id )
					return( addCharSet( i ) );

			throw new System.IO.IOException(
				"Character set of the database server is not supported");
		} // getCharSet


	/*
		** Name: getCharSet
		**
		** Description:
		**	Factory method for CharSet objects.  Determines mapping
		**	of character sets and encodings.  Maintains single CharSet
		**	per mapping.
		**
		** Input:
		**	charset	    Ingres character set name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	CharSet	    CharSet instance for character set.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		**	26-Dec-02 (gordy)
		**	    Extracted common functionality to addCharSet().
		*/
		
		public static CharSet getCharSet(String charset)
		{
			System.Int32 i;
			int cs;  // index into csInfo for the charset
			
			/*
			** Do we support the character set?
			*/
			try
			{
				if ((System.Object) (i = (System.Int32) csMap[charset]) != null)
					cs = i;// index into csInfo for the charset
				else
					throw new System.IO.IOException(
						"Character set of the database server is not supported");
			}
			catch (Exception)
			{
				throw new System.IO.IOException(
					"Character set of the database server is not supported");
			}

			return (addCharSet(i));
		}  // getCharSet


		/*
		** Name: getCharSet
		**
		** Description:
		**	Map character set ID or name to character encoding.
		**	If neither ID nor name is provided, default encoding will
		**	be used.
		**
		** Input:
		**	id	Character set numeric ID (0 for default).
		**	name	Character set name (null for default).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	CharSet	Character set/encoding.
		**
		** History:
		**	26-Dec-02 (gordy)
		**	    Extracted to separate method.
		*/

		public static CharSet
			getCharSet( int id, String name )
		{
			CharSet charset = null;

			try 
			{ 
				/*
				** Map numeric ID (if provided).
				*/
				if ( id != 0 )
					try 
					{ 
						charset = CharSet.getCharSet( id );
						return charset;
					}
					catch( Exception) 
					{
						/*
						** Attempt to use character set name if provided,
						** otherwise propogate the exception.
						*/
						if ( name == null )  throw;
					}
	
				/*
				** Map name (if needed. default if not provided).
				*/
				if (name == null)
					name = "ISO88591";  // default
				charset = CharSet.getCharSet( name ); 
			}
			catch( Exception)
			{
				throw new System.IO.IOException(
					"Character set of the database server " +
					"has no .NET character encoding");
			}

			return( charset );
		} // getCharSet


		/*
		** Name: addCharSet
		**
		** Description:
		**	Maintains character sets in the csInfo array.
		**
		** Input;
		**	cs	Index of character set in csInfo array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	CharSet	Character set associated with csInfo entry.
		**
		** History:
		**	26-Dec-02 (gordy)
		**	    Extracted from getCharSet() for common functionality.
		*/

		private static CharSet
		addCharSet( int cs )
		{
			/*
			** Only one CharSet created for each character set.
			*/
			if (csInfo[cs][3] == null)  // if CharSet not built yet
			{
				System.Text.Encoding encoding;
				
				/*
				** Is the encoding supported.
				*/
				if ((encoding = getEncoding((String) csInfo[cs][1])) == null)
					throw new System.IO.IOException(
						"Character set of the database server " +
						"has no .NET character encoding");
				
				csInfo[cs][3] =
					new CharSet(
						encoding,
						((bool)   csInfo[cs][2]),
						((string) csInfo[cs][0]),
						((string) csInfo[cs][1]));
			}
			
			return ((CharSet) csInfo[cs][3]);
		} // addCharSet


		/*
		** Name: ToString
		**
		** Description:
		**	Returns the encoding used by this CharSet.
		**
		** Input:
		**	None.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	String	Encoding.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public override String ToString()
		{
			return ((encoding == null)?
				"<default>":
				ingresName + " -> " +dotnetName + " (" + encoding.EncodingName + ")");
		}  // toString


		/*
		** Name: getSpaceChar
		**
		** Description:
		**	Returns the byte value representing a space character
		**	in the character set.  
		**
		**	Note: assumes all supported character sets have a space
		**	character represented by a single byte value.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte	The space character.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public byte
		getSpaceChar()
		{
			if (space == 0)
			{
				byte[] sc = getBytes(" ");
				if (sc.Length != 1)
				{
					space = 0x20;  // not found for some reason
				}
				else
					space = sc[0];
			}
			return (space);
		} // getSpaceChar


		/*
		** Name: getByteLength
		**
		** Description:
		**	Returns the length in bytes of a character array if it were 
		**	converted using the associated encoding.
		**
		**	For single-byte encodings a 1-to-1 mapping of Unicode characters
		**	and bytes is assumed.
		**
		**	Unfortunately, for multi-byte encodings the functionality
		**	provided by the API forces the conversion of the string
		**	to determine the resulting length: a hopelessly inefficient
		**	implementation.
		**
		** Input:
		**	ca	Character array.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	int	Length of converted string.
		**
		** History:
		**	25-Feb-03 (gordy)
		**	    Created.
		*/

		public int getByteLength( char[] ca )
		{
			return( multi_byte ?
				getByteLength( new String( ca ) ) : ca.Length );
		} // getByteLength


		/*
		** Name: getByteLength
		**
		** Description:
		**	Returns the length in bytes of the string if it were converted
		**	using the associated encoding.
		**
		**	For single-byte encodings a 1-to-1 mapping of Unicode characters
		**	and bytes is assumed.
		**
		**	Unfortunately, for multi-byte encodings the functionality
		**	provided by the API forces the conversion of the string
		**	to determine the resulting length: a hopelessly inefficient
		**	implementation.
		**
		** Input:
		**	str	String to be converted.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	int	Length of converted string.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public virtual int getByteLength(String str)
		{
			if (!multi_byte)
				return str.Length;
			else
				if (this.encoding == null)
				return System.Text.Encoding.UTF8.GetByteCount(str);
			else
				return this.encoding.GetByteCount(str);
		}  // getByteLength


		/*
		** Name: getBytes
		**
		** Description:
		**	Convert Unicode character array to byte array using 
		**	associated encoding.
		**
		** Input:
		**	ca	Character array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Converted string.
		**
		** History:
		**	25-Feb-03 (gordy)
		**	    Created.
		*/

		public byte[] getBytes( char[] ca )
		{
			return( getBytes( new String( ca ) ) );
		} // getBytes


		/*
		** Name: getBytes
		**
		** Description:
		**	Convert Unicode character sub-array to byte array using 
		**	associated encoding.
		**
		** Input:
		**	ca	Character array.
		**	offset	Offset of sub-array.
		**	length	Length of sub-array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Converted string.
		**
		** History:
		**	25-Feb-03 (gordy)
		**	    Created.
		*/

		public byte[] getBytes( char[] ca, int offset, int length )
		{
			return( getBytes( new String( ca, offset, length ) ) );
		} // getBytes


		/*
		** Name: getBytes
		**
		** Description:
		**	Convert Unicode string to byte string using associated encoding.
		**
		** Input:
		**	str	String to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Converted string.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public virtual byte[] getBytes(String str)
		{
			if (this.encoding == null)
				return System.Text.Encoding.UTF8.GetBytes(str);
			else
				return this.encoding.GetBytes(str);
		}  // getBytes
		
		
		/*
		** Name: getString
		**
		** Description:
		**	Convert byte string to Unicode string using associated encoding.
		**
		** Input:
		**	ba	String to be converted
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Converted string.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.*/
		
		public virtual String getString(byte[] ba)
		{
			return (getString(ba, 0, ba.Length));
		}  // getString
		
		
		/*
		** Name: getString
		**
		** Description:
		**	Convert byte string to Unicode string using associated encoding.
		**
		** Input:
		**	ba	Buffer holding string to be converted.
		**	offset	Starting index of string in buffer.
		**	length	Length of string.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Converted string.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public virtual String getString(byte[] ba, int offset, int length)
		{
			if (this.encoding == null)
				return System.Text.Encoding.UTF8.GetString(ba, offset, length);
			else
				return             this.encoding.GetString(ba, offset, length);
		}  // getString
		
		
		/*
		** Name: getISR
		**
		** Description:
		**	Returns an InputStreamReader for input from provided stream
		**	and conversion using the associated encoding .
		**
		** Input:
		**	stream		    InputStream to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStreamReader   Reader for encoding conversion of stream.
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public virtual InputStreamReader getISR(InputStream stream)
		{
			InputStreamReader isr;
			
			if (encoding == null)
				isr = new InputStreamReader(stream);
			else
				isr = new InputStreamReader(stream, encoding);
			
			return (isr);
		}  // getISR
		
		
		/*
		** Name: getOSW
		**
		** Description:
		**	Returns an OutputStreamWriter for conversion using the
		**	associated encoding and output to provided stream.
		**
		** Input:
		**	stream		    OutputStream to receive conversion.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	OutputStreamWriter  Writer for encoding conversion to stream
		**
		** History:
		**	 6-Sep-02 (gordy)
		**	    Created.
		**	11-Nov-02 (thoda04)
		**	    Ported to .NET Provider.
		*/

		public virtual OutputStreamWriter getOSW(OutputStream stream)
		{
			OutputStreamWriter osw;
			
			if (encoding == null)
				osw = new OutputStreamWriter(stream);
			else
				osw = new OutputStreamWriter(stream, encoding);
			
			return (osw);
		}  // getOSW


		/*
		** Name: getEncoding
		**
		** Description:
		**	Find the .NET character set Encoding corresponding to a
		**	.NET encodingName for the character set.
		**
		** Input:
		**	encodingName	    .NET encodingName or code page name (cp1234).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Encoding	    The .NET encoding.
		**
		** History:
		**	24-Jul-02 (thoda04)
		**	    Created.
		**	 7-Mar-07 (thoda04)
		**	    Added code page support.
		*/

		public static System.Text.Encoding getEncoding(String encodingName)
		{
			System.Text.Encoding encoding;
			
			/*
			** Map character set to encoding.  Force to
			** most general encoding if charset not found.
			*/
			if (encodingName == null)
				encodingName = "iso-8859-1";
			
			/*
			** Build the encoding object to be returned.
			*/
			try
			{
				if (encodingName.Length > 2 &&    // cp1234 codepage format?
					(encodingName[0] == 'c' || encodingName[0] == 'C') &&
					(encodingName[1] == 'p' || encodingName[1] == 'P'))
				{
					string s = encodingName.Substring(2);
					if (IsNumber(s))
					{
						int codepage = Int32.Parse(s);
						encoding = System.Text.Encoding.GetEncoding(codepage);
						return encoding;
					}
				}
				encoding = System.Text.Encoding.GetEncoding(encodingName);
			}
			catch (ArgumentException)      { return null; }
			catch (NotSupportedException)  { return null; }
			
			return (encoding);
		}  // getEncoding

		public static CharSet forName(String charset)
		{
			return getCharSet(charset);
		}

		internal CharsetEncoder newEncoder()
		{
			return new CharsetEncoder(this.encoding);
		}

		/// <summary>
		/// Test if a string is a number.  True if a non-negative integer.
		/// </summary>
		private static bool IsNumber(string token)
		{
			if (token == null || token.Length == 0)
				return false;

			foreach (char c in token)
			{
				if (!Char.IsDigit(c))
					return false;
			}
			return true;
		}

	}  // class CharSet

	internal class CharsetEncoder
	{
		System.Text.Encoding encoding;

		public CharsetEncoder(System.Text.Encoding _encoding)
		{
			this.encoding = _encoding;
		}

		public CharsetEncoder(int codePage)
		{ }

		public void onMalformedInput( CodingErrorAction action )
		{
			// ignore action
		}

		public void onUnmappableCharacter( CodingErrorAction action )
		{
			// ignore action
		}

		public CoderResult encode(
			CharBuffer inBuff, ByteBuffer outBuff, bool endOfInput )
		{
			int oldPosition;
			byte[] ba = new byte[32];
			char[] ca = new char[1];

			while(inBuff.hasRemaining())
			{
				oldPosition = inBuff.position();  // save the input position
				ca[1] = inBuff.get();             // get the next character
				int count = encoding.GetBytes(ca, 0, 1, ba, 0);
				if (count > outBuff.remaining())
				{
					inBuff.position(oldPosition);
					return CoderResult.OVERFLOW;
				}
				outBuff.put(ba, 0, count);  // move the bytes into ByteBuffer
			}
			return CoderResult.UNDERFLOW;
		}

		public CoderResult flush(ByteBuffer outBuff)
		{
			return CoderResult.UNDERFLOW;
		}


	}

	internal enum CodingErrorAction
	{
		REPLACE
	}

	internal enum CoderResult
	{
		/// <summary>
		/// Insufficient room in the output buffer.
		/// </summary>
		OVERFLOW,
		/// <summary>
		/// Input buffer has been completely consumed.
		/// </summary>
		UNDERFLOW
	}

	/*
	** Name: CharArrayReader
	**
	** Description:
	**	Create a reader from a character array.
	**
	**  Methods Overriden
	**
	**	Read	    Read stream data.
	**	skip	    Skip stream data.
	**	Close	    Close stream and notify result set.
	**	flush	    Close stream without notifying result set.
	**
	**  Protected Data
	**
	**	in	    BLOB Reader stream.
	**	closed	    Is stream closed?
	**
	** History:
	**	 4-Sep-02 (thoda04)
	**	    Created.
	*/
	
internal class CharArrayReader : Reader
{
	private bool closed      = false;
	private bool end_of_data = false;
	private char[] inChars   = null;
	private int    inCharsRead = 0;
	private char[] ca = new char[1];   // work area to read a single char
	private int    maxlength;
	
	/*
		** Name: CharArrayReader
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	inChars	input char array to build stream reader around.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 4-Sep-02 (thoda04)
		**	    Created.
	*/
	
	public CharArrayReader(char[] inChars)
	{
		if (inChars == null)
			throw new System.ArgumentNullException();

		this.inChars = inChars;
		this.maxlength = inChars.Length;

	} // CharArrayReader
	
	
	public CharArrayReader(char[] inChars, int maxlength) : this(inChars)
	{
		if (maxlength > 0)
			this.maxlength = Math.Min( inChars.Length, maxlength );
	} // CharArrayReader
	
	
	/*
		** Name: Read
		**
		** Description:
		**	Read one character from the character stream.
		**
		** Input:
		**	None
		**
		** Output:
		**	None
		**
		** Returns:
		**	character read, or -1 if end-of-data.
		**
		** History:
		**	 4-Sep-02 (thoda04)
		**	    Created.
		*/
	
	public override int Read()
	{
		int len = Read(ca, 0, 1);
		return (len == 1?(int) ca[0] & 0xffff:-1);
	} // Read
			
			
	/*
		** Name: Read(char[] cbuf)
		**
		** Description:
		**	Read a character array from the char stream.
		**
		** Input:
		**	None
		**
		** Output:
		**	cbuf	Character array.
		**
		** Returns:
		**	int	Number of characters read, or -1 if end-of-data.
		**
		** History:
		**	 4-Sep-02 (thoda04)
		**	    Created.
		*/
			
	public int Read(char[] cbuf)
	{
		return (Read(cbuf, 0, cbuf.Length));
	}


	/*
		** Name: Read (char[] cbuf, int offset, int length)
		**
		** Description:
		**	Read a character sub-array from the segment stream.
		**
		** Input:
		**	offset	Offset in character array.
		**	length	Number of characters to read.
		**
		** Output:
		**	cbuf	Character array.
		**
		** Returns:
		**	int	Number of characters read, or -1 if end-of-data.
		**
		** History:
		**	 4-Sep-02 (thoda04)
		**	    Created.
		*/
			
	public override int Read(char[] cbuf, int offset, int length)
	{
		int len, total = 0;

		if (closed)
			throw new System.IO.IOException();

		if (length > 0 && !end_of_data)
		{
			len = System.Math.Min(length, this.maxlength - inCharsRead);
			if (len == 0)
			{
				end_of_data = true;
				return (-1);
			}

			for (int end = offset + len; offset < end;)
				cbuf[offset++] = inChars[inCharsRead++];

			total += len;
		}

		return (total > 0?total:  // if chars were read then return total
			(end_of_data?-1:0));
	} // Read


	/*
		** Name: Close
		**
		** Description:
		**	Close stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 4-Sep-02 (thoda04)
		**	    Created.
	*/
	
	public override void  Close()
	{
		this.closed = true;
		this.inChars = null;
	} // Close

} // class CharArrayReader


	internal class Buffer
	{
		protected int _capacity = 0;
		protected int _limit    = 0;
		protected int _position = 0;

		protected Buffer(int count)
		{
			_capacity = count;
		}

		public int capacity()  { return _capacity; }
		public int limit()     { return _limit; }
		public int position()  { return _position; }

		public Buffer position( int newPosition)
		{
			if (newPosition < 0  ||  newPosition > _limit)
				throw new ArgumentException(
					"SqlStream.Buffer.position(newPosition) exceeds limit");
			_position = newPosition;
			return this;
		}

		public Buffer clear()
		{
			_position = 0;
			_limit    = _capacity;
			return this;
		}

		public Buffer flip()
		{
			_limit    = _position;
			_position = 0;
			return this;
		}

		public bool hasRemaining()
		{
			return (_position < _limit);
		}

		public int remaining()
		{
			return (_limit - _position);
		}

	}  //  Buffer


	internal class ByteBuffer : Buffer
	{
		private byte[] buf;

		private ByteBuffer(int count) : base( count )
		{
			buf = new byte[ count ];
		}

		public static ByteBuffer allocate(int count)
		{
			return new ByteBuffer( count );
		}

		public byte[] array()
		{
			byte[] newbuf = new Byte[_position];

			for(int i = 0; i < _position; i++)
				newbuf[i] = buf[i];

			return newbuf;
		}

		public byte get()
		{
			return buf[_position++];
		}

		public ByteBuffer get(byte[] dst, int offset, int count)
		{
			for (int i = offset; i < offset + count; i++)
				dst[i] = this.get();
			return this;
		}

		public ByteBuffer put(byte[] ba, int offset, int count)
		{
			count += offset;
			for (int i = offset; i < count; i++)
				buf[_position++] = ba[i];

			return this;
		}

	}  // ByteBuffer


	internal class CharBuffer : Buffer
	{
		private char[] buf;

		private CharBuffer(int count) : base( count )
		{
			buf = new char[ count ];
		}

		public static CharBuffer allocate(int count)
		{
			return new CharBuffer( count );
		}

		public char[] array()
		{
			char[] newbuf = new Char[_position];

			for(int i = 0; i < _position; i++)
				newbuf[i] = buf[i];

			return newbuf;
		}

		public char get()
		{
			return buf[_position++];
		}

		public CharBuffer get(char[] dst, int offset, int count)
		{
			for (int i = offset; i < offset + count; i++)
				dst[i] = this.get();
			return this;
		}

		public CharBuffer put(char c)
		{
			buf[_position++] = c;
			return this;
		}

	}  // CharBuffer

}  // namespace
