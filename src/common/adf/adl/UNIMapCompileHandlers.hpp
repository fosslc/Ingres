/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: UNIMapCompileHandlers.cpp
**
** Description:
**    SAX Handler functions, invoked by the SAX XML parser, to compile
**    the mapping file for codepage-unicode conversion, in xml format
**    into binary form for later reload.
**
** Extended Description:
**    This module contains populated SAX Handler functions, invoked
**    by the SAX XML parser, required to compile the mapping file in xml
**    format into binary form. for later reload into memory for searching
**    the translation map file . The format and dtd for
**    alias files is provided at http://www.unicode.org/reports/tr22.
**
** History:
**      14-Jan-2003 (gupsh01)
**          Created.
**	15-Mar-2004 (gupsh01)
**	    Modified to support Xerces 2.3.
**      18-aug-2004 (devjo01)
**          Remove local include of Ingres header files in order to
**          avoid compile error in some build environments..
**	01-May-2009 (bonro01)
**	    Add flag variable to align ucharmap assignments section.
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
#include    <xercesc/sax/HandlerBase.hpp>
#include    <xercesc/framework/XMLFormatter.hpp>

/*
 * $Log: SAX2PrintHandlers.hpp,v $
 * Revision 1.2  2000/08/09 22:20:39  jpolast
 * updates for changes to sax2 core functionality.
 *
 * Revision 1.1  2000/08/02 19:16:14  jpolast
 * initial checkin of SAX2Print
 *
 *
 */
/*
** History
**	14-jan-2004 (gupsh01)
**       created. 
** 
*/

/* Define initial buffer limits */ 
#define MAX_VALID_TBL_SIZE		sizeof(ADU_MAP_VALIDITY) * 25
#define MAX_ASSIGNMENT_SIZE		sizeof(ADU_MAP_ASSIGNMENT) * 16000
#define	MAX_MAPPINGTBL_SIZE 		sizeof(ADU_MAP_HEADER) + MAX_VALID_TBL_SIZE + MAX_ASSIGNMENT_SIZE

#if XERCES_VERSION_MAJOR < 3
typedef const unsigned int INGXMLSize_t;
# else
typedef const XMLSize_t INGXMLSize_t;
#endif

XERCES_CPP_NAMESPACE_USE

class UNIMapCompileHandlers : public HandlerBase, private XMLFormatTarget
{
public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    UNIMapCompileHandlers
    (
        const   char* const			encodingName, 
	const XMLFormatter::UnRepFlags		unRepFlags, 
	const bool				expandNamespaces,
	char					*outputFile
    );
    ~UNIMapCompileHandlers(){}


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
        , INGXMLSize_t    count
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
        , INGXMLSize_t    length
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

	i4			buffer_size;
	i4			validity_cnt;
	i4			assignment_cnt;
	SIZE_TYPE		*validity_size_ptr;
	SIZE_TYPE		*assignment_size_ptr;
	u_i4			*subchar_ptr;
	u_i2			*subchar1_ptr;
	u_i4			*tbl_size_ptr;

	u_i4			map_tbl_size;
 	char 			*map_tbl;
 	char 			*charset;
	char			*tptr;	
	bool			align_validities;
};
