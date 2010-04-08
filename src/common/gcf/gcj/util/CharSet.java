/*
** Copyright (c) 2000, 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: CharSet.java
**
** Description:
**
**	Defines a class which provides mapping and conversion between
**	DBMS installation character sets, Java character encodings and
**	Unicode.
**
**  Classes
**
**	CharSet
**
** History:
**	21-Apr-00 (gordy)
**	    Created.
**	16-Aug-02 (gordy)
**	    Added mappings for new character sets.
**	 6-Sep-02 (gordy)
**	    Localized conversion in this class.
**	11-Sep-02 (gordy)
**	    Moved to GCF.
**	26-Dec-02 (gordy)
**	    Added factory method taking char-set ID parameter.
**	26-Jan-03 (weife01) Bug 109535
**	    Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
**	25-Feb-03 (gordy)
**	    Added methods to handle character arrays in addition to strings.
**	 1-Dec-03 (gordy)
**	    Added access to char-set space character.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
*/

import	java.util.Hashtable;
import	java.io.InputStream;
import	java.io.ByteArrayInputStream;
import	java.io.InputStreamReader;
import	java.io.OutputStream;
import	java.io.OutputStreamWriter;
import	java.io.UnsupportedEncodingException;


/*
** Name: CharSet
**
** Description:
**	Maps DBMS character sets to Java character encodings
**	and provides conversion between Unicode and byte encoded
**	strings.
**
**  Public Methods:
**
**	getCharSet	Factory for CharSet mappings.
**	getSpaceChar	Returns space character.
**	getByteLength	Returns length of converted string.
**	getBytes	Convert Unicode to bytes.
**	getString	Convert bytes to Unicode.
**	getISR		Get InputStreamReader for encoding.
**	getOSW		Get OutputStreamWriter for encoding.
**
**  Private Data:
**
**	enc		Encoding for CharSet instance.
**	multi_byte	Is encoding multi-byte.
**	space		Space characgter in char-set.
**
**	def_enc		Default encoding used by Java system.
**	csMap		Maps charsets to encodings.
**	csInfo		Charset/encoding mappings.
**	csId		Charset ID mappings.
**
**  Private Methods:
**
**	addCharSet	Maintains character sets.
**	checkEncoding	Validate encoding, return canonical encoding.
**
** History:
**	21-Apr-00 (gordy)
**	    Extracted from class EdbcObject.
**	16-Aug-02 (gordy)
**	    Added mappings for new character sets.
**	 6-Sep-02 (gordy)
**	    CharSet object now supports conversion for selected encoding.
**	26-Dec-02 (gordy)
**	    CharSet's can now be obtained via the char-set ID.  Added
**	    csId and addCharSet().
**	26-Jan-03 (weife01) Bug 109535
**	    Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
**	25-Feb-03 (gordy)
**	    Added methods to handle character arrays in addition to strings.
**	 1-Dec-03 (gordy)
**	    Added space and getSpaceChar().
** 	10-Jul-04 (rajus01) Bug  112452
**	    Cross integrated Fei's prior changes to CharSet.java in 
**	    edbc!main codeline. 
*/

public class
CharSet
{
    
    private String		enc;		// Java character encoding
    private boolean		multi_byte;	// Is encoding multi-byte?
    private byte		space = 0;	// Space character in char-set
    
    private static String	def_enc = null; // default encoding
    
    /*
    ** DBMS character set to Java character encoding mappings.
    ** Each mapping may have an associated CharSet instance which
    ** is created by the getCharSet() factory method.  The multi-
    ** byte indication permits optimization in getByteLength().
    */
    private static final Hashtable	    csMap = new Hashtable();
    private static final Object		    csInfo[][] = 
    {
    //	  Ingres charset	Java Encoding	Multi-byte	CharSet
	{ "ISO88591",		"ISO-8859-1",	Boolean.FALSE,	null },
	{ "ISO88592",		"ISO8859_2",	Boolean.FALSE,	null },
	{ "ISO88595",		"ISO8859_5",	Boolean.FALSE,	null },
	{ "ISO88599",		"ISO8859_9",	Boolean.FALSE,	null },
	{ "IS885915",		"ISO8859_15",	Boolean.FALSE,	null },
	{ "IBMPC_ASCII_INT",	"Cp850",	Boolean.FALSE,	null },
	{ "IBMPC_ASCII",	"Cp850",	Boolean.FALSE,	null },
	{ "IBMPC437",		"Cp437",	Boolean.FALSE,	null },
	{ "ELOT437",		"Cp737",	Boolean.FALSE,	null },
	{ "SLAV852",		"Cp852",	Boolean.FALSE,	null },
	{ "IBMPC850",		"Cp850",	Boolean.FALSE,	null },
	{ "CW",			"Cp1251",	Boolean.FALSE,	null },
	{ "ALT",		"Cp855",	Boolean.FALSE,	null },
	{ "PC857",		"Cp857",	Boolean.FALSE,	null },
	{ "WIN1250",		"Cp1250",	Boolean.FALSE,	null },
	{ "KOI8",		"KOI8_R",	Boolean.FALSE,	null },
	{ "IBMPC866",		"Cp866",	Boolean.FALSE,	null },
	{ "WIN1252",		"Cp1252",	Boolean.FALSE,	null },
	{ "ASCII",		"US-ASCII",	Boolean.FALSE,	null },
	{ "DECMULTI",		"ISO-8859-1",	Boolean.FALSE,	null },
	{ "HEBREW",		"Cp424",	Boolean.FALSE,	null },
	{ "THAI",		"Cp874",	Boolean.FALSE,	null },
	{ "GREEK",		"Cp875",	Boolean.FALSE,	null },
	{ "HPROMAN8",		"ISO-8859-1",	Boolean.FALSE,	null },
	{ "ARABIC",		"Cp420",	Boolean.FALSE,	null },
	{ "WHEBREW",		"Cp1255",	Boolean.FALSE,	null },
	{ "PCHEBREW",		"Cp862",	Boolean.FALSE,	null },
	{ "WARABIC",		"Cp1256",	Boolean.FALSE,	null },
	{ "DOSASMO",		"Cp864",	Boolean.FALSE,	null },
	{ "WTHAI",		"Cp874",	Boolean.FALSE,	null },
	{ "EBCDIC_C",		"Cp500",	Boolean.FALSE,	null },
	{ "EBCDIC_ICL",		"Cp500",	Boolean.FALSE,	null },
	{ "EBCDIC_USA",		"Cp037",	Boolean.FALSE,	null },
	{ "EBCDIC",		"Cp500",	Boolean.FALSE,	null },
	{ "CHINESES",		"Cp1383",	Boolean.TRUE,	null },
 	{ "KOREAN",		"Cp949",	Boolean.TRUE,	null },
 	{ "KANJIEUC",		"EUC_JP",	Boolean.TRUE,	null },
 	{ "SHIFTJIS",		"SJIS",		Boolean.TRUE,	null },
 	{ "CHTBIG5",		"Big5",		Boolean.TRUE,	null },
 	{ "CHTEUC",		"EUC_TW",	Boolean.TRUE,	null },
 	{ "CHTHP",		"EUC_TW",	Boolean.TRUE,	null },
 	{ "CSGBK",		"GBK",		Boolean.TRUE,	null },
 	{ "CSGB2312",		"EUC_CN",	Boolean.TRUE,	null },
	{ "UTF8",		"UTF-8",	Boolean.TRUE,	null },
    };

    /*
    ** Maps the Ingres character-set ID to a csInfo entry.  Position in
    ** array must be same as position in csInfo array.
    */
    private static final int		    csId[] =
    {
	/* ISO88591 */	0x0001,		/* ISO88592 */	0x0002,
	/* ISO88595 */	0x0005,		/* ISO88599 */	0x0009,
	/* IS885915 */	0x000F,		/* IBMPC_ASCII_INT */ 0x0010,
	/* IBMPC_ASCII */ 0x0010,	/* IBMPC437 */	0x0011,
	/* ELOT437 */	0x0012,		/* SLAV852 */	0x0013,
	/* IBMPC850 */	0x0014,		/* CW */	0x0015,
	/* ALT */	0x0016,		/* PC857 */	0x0017,
	/* WIN1250 */	0x0018,		/* KOI8 */	0x0019,
	/* IBMPC866 */	0x001B,		/* WIN1252 */	0x001C,
	/* ASCII */	0x0020,		/* DECMULTI */	0x0030,
	/* HEBREW */	0x0031,		/* THAI */	0x0032,
	/* GREEK */	0x0033,		/* HPROMAN8 */	0x0040,
	/* ARABIC */	0x0050,		/* WHEBREW */	0x0060,
	/* PCHEBREW */	0x0061,		/* WARABIC */	0x0062,
	/* DOSASMO */	0x0063,		/* WTHAI */	0x0064,
	/* EBCDIC_C */	0x0100,		/* EBCDIC_ICL*/	0x0101,
	/* EBCDIC_USA*/	0x0102,		/* EBCDIC */	0x0102,
	/* CHINESES */	0x0200,		/* KOREAN */	0x0201,
 	/* KANJIEUC */	0x0202,		/* SHIFTJIS */	0x0203,
 	/* CHTBIG5 */	0x0205,		/* CHTEUC */	0x0206,
 	/* CHTHP */	0x0207,		/* CSGBK */	0x0208,
 	/* CSGB2312 */	0x0209,		/* UTF8 */	0x0311,
    };


/*
** Name: <class initializer>
**
** Description:
**	Complete the initialization of static fields.
**
** History:
**	21-Apr-00 (gordy)
**	    Extracted from EdbcObject.
**	 6-Sep-02 (gordy)
**	    Moved encoding determination to checkEncoding(), hash table
**	    value is now array index for character set mapping.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Use more efficient value constructor.
*/

static
{
    /*
    ** Load the character set/encoding mappings.
    */
    def_enc = checkEncoding( null );	    // Get default encoding

    for( int i = 0; i < csInfo.length; i++ )
	csMap.put( csInfo[ i ][ 0 ], Integer.valueOf( i ) );
} // static


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

public static CharSet
getCharSet( int cs_id )
    throws UnsupportedEncodingException
{
    for( int i = 0; i < csId.length; i++ )
	if ( csId[ i ] == cs_id )  return( addCharSet( i ) );

    throw new UnsupportedEncodingException();
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
**	26-Dec-02 (gordy)
**	    Extracted common functionality to addCharSet().
*/

public static CharSet
getCharSet( String charset )
    throws UnsupportedEncodingException
{
    Integer i;

    /*
    ** Do we support the character set?
    */
    if ( (i = (Integer)csMap.get( charset )) == null )
	throw new UnsupportedEncodingException();
    
    return( addCharSet( i.intValue() ) );
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
    throws UnsupportedEncodingException
{
    /*
    ** Only one CharSet created for each character set.
    */
    if ( csInfo[ cs ][ 3 ] == null )
    {
	String  enc;

	/*
	** Is the encoding supported.
	*/
	if ( (enc = checkEncoding( (String)csInfo[ cs ][ 1 ] )) == null )
	    throw new UnsupportedEncodingException();

	/*
	** Some overhead can be avoided by using
	** the default encoding methods instead
	** of explicit encodings when possible.
	*/
	if ( enc.equals( def_enc ) )  enc = null;

	csInfo[ cs ][ 3 ] = 
	    new CharSet( enc, ((Boolean)csInfo[ cs ][ 2 ]).booleanValue() );
    }

    return( (CharSet)csInfo[ cs ][ 3 ] );
} // addCharSet


/*
** Name: checkEncoding
**
** Description:
**	Validates that an encoding is supported by the current Java
**	implementation.  Returns the canonical form of the encoding.
**
** Input:
**	enc	Encoding.
**
** Ouptut:
**	None.
**
** Returns:
**	String	Canonical encoding.
**
** History:
**	 6-Sep-02 (gordy)
**	    Created.
*/

private static String
checkEncoding( String enc )
{
    try
    {
        byte		    ba[] = { 0x00 };	
        InputStream	    is = new ByteArrayInputStream( ba );
	InputStreamReader   isr;

	if ( enc == null )
	    isr = new InputStreamReader( is );
	else
	    isr = new InputStreamReader( is, enc );

	enc = isr.getEncoding();
    }
    catch( UnsupportedEncodingException ex ) 
    { 
	enc = null; 
    }

    return( enc );
} // checkEncoding


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
    this.enc = null;		// Default encoding
    this.multi_byte = true;	// TODO: Is there a way to tell?
    return;
} // CharSet


/*
** Name: CharSet
**
** Description:
**	Class constructor for specific encoding.
**
** Input:
**	enc	Encoding.
**
** Ouptut:
**	None.
**
** Returns:
**	None.
**
** History:
**	26-Dec-02 (gordy)
**	    Created.
*/

public
CharSet( String enc )
    throws UnsupportedEncodingException
{
    /*
    ** Validate encoding
    */
    if ( (enc = checkEncoding( enc )) == null )
	throw new UnsupportedEncodingException();
    
    this.enc = enc;
    this.multi_byte = true;	// TODO: Is there a way to tell?
    return;
}


/*
** Name: CharSet
**
** Description:
**	Class constructor for factory methods.
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
**	 6-Sep-02 (gordy)
**	    Added instance parameters.
*/

private
CharSet( String enc, boolean multi_byte )
{
    this.enc = enc;
    this.multi_byte = multi_byte;
    return;
} // CharSet


/*
** Name: toString
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
*/

public String
toString()
{
    return( (enc == null) ? "<default>" : enc );
} // toString


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
    throws UnsupportedEncodingException
{
    if ( space == 0 )
    {
	byte sc[] = getBytes( " " );
	if ( sc.length != 1 )  
	    throw new UnsupportedEncodingException("Invalid space character");
	space = sc[ 0 ];
    }
    return( space );
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
**	provided by the Java API forces the conversion of the string
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

public int
getByteLength( char ca[] )
{
    return( multi_byte ? getByteLength( new String( ca ) ) : ca.length );
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
**	provided by the Java API forces the conversion of the string
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
*/

public int
getByteLength( String str )
{
    byte    ba[];
    int	    length;

    if ( ! multi_byte )	    // One character => one byte
	length = str.length(); 
    else  try
    {
	/*
	** Convert string and get length
	*/
        if ( enc == null )
	    ba = str.getBytes();
        else
	    ba = str.getBytes( enc );

        length = ba.length;
    }
    catch( UnsupportedEncodingException ex ) 
    { 
	// should not happen!
	length = str.length(); 
    }

    return( length );
} // getByteLength


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

public byte[]
getBytes( char ca[] )
    throws UnsupportedEncodingException
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

public byte[]
getBytes( char ca[], int offset, int length )
    throws UnsupportedEncodingException
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
*/

public byte[]
getBytes( String str )
    throws UnsupportedEncodingException
{
    byte ba[];

    if ( enc == null )
	ba = str.getBytes();
    else
	ba = str.getBytes( enc );

    return( ba );
} // getBytes


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
**	    Created.
*/

public String
getString( byte ba[] )
    throws UnsupportedEncodingException
{
    return( getString( ba, 0, ba.length ) );
} // getString


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
*/

public String
getString( byte ba[], int offset, int length )
    throws UnsupportedEncodingException
{
    String str;

    if ( enc == null )
	str = new String( ba, offset, length );
    else
	str = new String( ba, offset, length, enc );

    return( str );
} // getString


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
*/

public InputStreamReader
getISR( InputStream stream )
    throws UnsupportedEncodingException
{
    InputStreamReader	isr;

    if ( enc == null )
	isr = new InputStreamReader( stream );
    else
	isr = new InputStreamReader( stream, enc );

    return( isr );
} // getISR


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
*/

public OutputStreamWriter
getOSW( OutputStream stream )
    throws UnsupportedEncodingException
{
    OutputStreamWriter	osw;

    if ( enc == null )
	osw = new OutputStreamWriter( stream );
    else
	osw = new OutputStreamWriter( stream, enc );

    return( osw );
} // getOSW


} // class CharSet
