/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: CharSet.java
**
** Description:
**
**	Defines a class which provides mapping and conversion between
**	Ingres installation character sets, Java characeter encodings
**	and Unicode.
**
** History:
**	21-Apr-00 (gordy)
**	    Created.
**	16-Aug-02 (gordy)
**	    Added mappings for new character sets.
**	 6-Sep-02 (gordy)
**	    Localized conversion in this class.
**      26-Jan-03 (weife01) Bug 109535
**          Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
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
**	Maps Ingres character sets to Java character encodings
**	and provides conversion between Unicode and byte encoded
**	strings.
**
**  Public Methods:
**
**	getCharSet	Factory for CharSet mappings.
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
**	def_enc		Default encoding used by Java system.
**	csMap		Maps charsets to encodings.
**	csInfo		Charset/encoding mappings.
**
**  Private Methods:
**
**	checkEncoding	Validate encoding, return canonical encoding.
**
** History:
**	21-Apr-00 (gordy)
**	    Extracted from class EdbcObject.
**	16-Aug-02 (gordy)
**	    Added mappings for new character sets.
**	 6-Sep-02 (gordy)
**	    CharSet object now supports conversion for selected encoding.
**      16-Jan-03 (loera01) Bug 109484
**          Map ISO885915 to ISO8859_15_FDIS.
**      26-Jan-03 (weife01) Bug 109535
**          Map EBCDIC and EBCDIC_C to Cp500, and EBCDIC_USA to Cp037.
** 	10-Jul-04 (rajus01) Bug  112452
**	    Cross integrated Fei's prior changes to CharSet.java in 
**	    edbc!main codeline. 
*/

public class
CharSet
{
    
    private String		enc;		// Java character encoding
    private boolean		multi_byte;	// Is encoding multi-byte?

    private static String	def_enc = null; // default encoding
    
    /*
    ** Ingres character set to Java character encoding mappings.
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
	{ "IS885915",		"ISO8859_15_FDIS",Boolean.FALSE,null },
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
*/

static
{
    /*
    ** Load the character set/encoding mappings.
    */
    def_enc = checkEncoding( null );	    // Get default encoding

    for( int i = 0; i < csInfo.length; i++ )
	csMap.put( csInfo[ i ][ 0 ], new Integer( i ) );
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
*/

public static CharSet
getCharSet( String charset )
    throws UnsupportedEncodingException
{
    Integer i;
    int	    cs;

    /*
    ** Do we support the character set?
    */
    if ( (i = (Integer)csMap.get( charset )) != null )
	cs = i.intValue();
    else
	throw new UnsupportedEncodingException();

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
} // getCharSet


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
**	 6-Sep-02 (gordy)
**	    Added instance parameters.
*/

private
CharSet( String enc, boolean multi_byte )
{
    this.enc = enc;
    this.multi_byte = multi_byte;
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

} // class CharSet
