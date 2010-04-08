/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: UNIAlsCompileHandlers.hpp - handler header file for unialscompile.
**
** Description:
**    SAX Handler functions, invoked by the SAX XML parser, to compile
**    the alias file for character sets in xml format into binary form
**    for later reload into memory for searching the translation map file
**    for an alias.
** Extended Description:
**    This module contains populated SAX Handler functions, invoked
**    by the SAX XML parser, required to compile the alias file in xml
**    format into binary form for later reload into memory for searching
**    the translation map file for an alias. The format and dtd for
**    alias files is provided at http://www.unicode.org/reports/tr22.
**
** History:
**      15-Jan-2004 (gupsh01)
**          Created.
**	29-Jan-2004 (gupsh01)
**	    Reduced the size of buffer allocated for aliasmaptbl. This
**	    may have to be increased when more alias data is added to the 
**	    table.
**	15-Mar-2004 (gupsh01)
**	    Modified to support Xerces 2.3.
**	21-Sep-2009 (hanje04)
**	   Use new include method for stream headers on OSX (DARWIN) to
**	   quiet compiler warnings.
**	   Also add version specific definitions of parser funtions to
**	   allow building against Xerces 2.x and 3.x
**	25-Jan-2010 (kschendel) b123194
**	    V3 variant of INGXMLSize_t needs const to match Xerces.
*/

/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: SAX2PrintHandlers.hpp,v $
 * Revision 1.2  2000/08/09 22:20:39  jpolast
 * updates for changes to sax2 core functionality.
 *
 * Revision 1.1  2000/08/02 19:16:14  jpolast
 * initial checkin of SAX2Print
 */
/*
** History
**	14-jan-2004 (gupsh01)
**       created. 
** 
*/
# include <xercesc/sax/HandlerBase.hpp>
# include <xercesc/framework/XMLFormatter.hpp>

#define MAX_ALSTAB_SIZE (sizeof(i4) * 4 + sizeof(ADU_ALIAS_DATA) * 400 + sizeof(ADU_ALIAS_MAPPING) * 100)

#if XERCES_VERSION_MAJOR < 3
typedef const unsigned int INGXMLSize_t;
# else
typedef const XMLSize_t INGXMLSize_t;
#endif

XERCES_CPP_NAMESPACE_USE

class UNIAlsCompileHandlers : public HandlerBase, private XMLFormatTarget
{
public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    UNIAlsCompileHandlers
    (
        const   char* const                 encodingName
        , const XMLFormatter::UnRepFlags    unRepFlags
		, const bool						expandNamespaces
    );
    ~UNIAlsCompileHandlers();


    // -----------------------------------------------------------------------
    //  Implementations of the format target interface
    // -----------------------------------------------------------------------
    void writeChars
    (
        const   XMLByte* const  toWrite
    );

    void writeChars
    (
        const   XMLByte* const  toWrite
        , INGXMLSize_t count
        , XMLFormatter* const   formatter
    );


    // -----------------------------------------------------------------------
    //  Implementations of the SAX DocumentHandler interface
    // -----------------------------------------------------------------------
    void endDocument();

    void endElement( const XMLCh* const name);

    void characters(const XMLCh* const chars, INGXMLSize_t length);

    void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , INGXMLSize_t length
    );

    void processingInstruction
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    void startDocument();

    void startElement(	const   XMLCh* const    name,
	 					AttributeList&	attributes);



    // -----------------------------------------------------------------------
    //  Implementations of the SAX ErrorHandler interface
    // -----------------------------------------------------------------------
    void warning(const SAXParseException& exception);
    void error(const SAXParseException& exception);
    void fatalError(const SAXParseException& exception);



    // -----------------------------------------------------------------------
    //  Implementation of the SAX DTDHandler interface
    // -----------------------------------------------------------------------
    void notationDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
    );

    void unparsedEntityDecl
    (
        const   XMLCh* const    name
        , const XMLCh* const    publicId
        , const XMLCh* const    systemId
        , const XMLCh* const    notationName
    );

	void normalize(char *instring, i4 inlength, char *outstring);
	STATUS writedata();

private :
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fFormatter
    //      This is the formatter object that is used to output the data
    //      to the target. It is set up to format to the standard output
    //      stream.
    // -----------------------------------------------------------------------
    XMLFormatter    fFormatter;
	bool			fExpandNS ;
	ADU_ALIAS_DATA		alAliasArray[900];
	ADU_ALIAS_MAPPING	alMappingArray[250];
 	i4			mapindex;
 	i4			aliasindex;
 	i4			mapcount;
 	i4			aliascount;
};
