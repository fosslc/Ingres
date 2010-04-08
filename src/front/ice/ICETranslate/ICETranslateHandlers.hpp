/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
 * The Apache Software License, Version 1.1
 *
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
 *
 *
 */
/*
** History (CA)
**     13-Mar-2001 (peeje01)
**         SIR 103810
**         Created.
**     01-Jun-2001 (peeje01)
**         SIR 103810
**         Upgrade to Xerces 1.4: add new writeChar overload
**     30-Oct-2001 (peeje01)
**         BUG 105899
**         Correct handling of long runs of character data and protect
**         stack from overflows
**      12-Mar-2004 (gupsh01)
**         Modified for supporting Xerces 2.3. 
**	1-Feb-2010 (kschendel for drivi01) b123194
**	    Define INGXMLSize_t for Xerces 3.x compatibility.
** 
*/
	
#include    <xercesc/sax2/DefaultHandler.hpp>
#include    <xercesc/framework/XMLFormatter.hpp>

// peeje01: stack additions
// Stack .h files should be in II_SYSTEM!ingres!files:
# include <icestack.h>
# include <icegen.h>

# include <compat.h>
# include <cv.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <st.h>

# include <gl.h>
# include <iicommon.h>
# include <fe.h>

// ICE Additions
# define	MAX_OLD_ICE_STATEMENT_LENGTH	1000
# define        ICE_STATE_MAX 1000    // This should be way too big. Proffessional XML says 10 should do...
# define        ICE_STACK_SIZE 10000
# define	ICE_DECLARE	1
# define	ICE_VAR		12
# define        ICE_ROOT_LVL   0
# define        ICE_EXTEND_ROOT 0
# define        ICE_CHARACTERS_SIZE 5000

#if XERCES_VERSION_MAJOR < 3
typedef const unsigned int INGXMLSize_t;
#else
typedef const XMLSize_t INGXMLSize_t;
#endif

XERCES_CPP_NAMESPACE_USE

class SAX2ICETranslate : public DefaultHandler, private XMLFormatTarget
{
public:
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    SAX2ICETranslate
    (
        const   char* const                 encodingName
        , const XMLFormatter::UnRepFlags    unRepFlags
		, const bool						expandNamespaces
    );
    ~SAX2ICETranslate();


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

    void endElement( const XMLCh* const uri, 
					 const XMLCh* const localname, 
					 const XMLCh* const qname);

    void characters(const XMLCh* const chars, const unsigned int length);

    void ignorableWhitespace
    (
        const   XMLCh* const    chars
        , const unsigned int    length
    );

    void processingInstruction
    (
        const   XMLCh* const    target
        , const XMLCh* const    data
    );

    void startDocument();

    void startElement(	const   XMLCh* const    uri,
						const   XMLCh* const    localname,
						const   XMLCh* const    qname,
					    const   Attributes&		attributes);



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
	// ICE Additions
    int				ICELevel;
    int             ICENextLevel;
    int             ICEFlush;                   // Flag: if not 'OK' do not flush the stack (for i3ce_extend)
    int             ice_parameter_count;		// Counter to test if first parameter to a function 0 = Yes, and that the parameter list is ended
	int				ice_header_count;			// Counter for elements in a header list
	int				ice_link_count;				// Counter for elements in a links list
    int             ice_state[ICE_STATE_MAX];	// ICE State register per ICE Level
//Remember stack status to avoid pushing on an already full stack
    int             ice_stack_status;

	char * attributecp(char *target, XMLCh *attribute_value);

	// ICE Additions

    // Helper Functions
// add_attribute
	static int ice_add_attribute(char *oldICE_p, const wchar_t *attributeName, const Attributes& attributeList);
// power2
	static int ice_power2(const int n);
// graves
	static int ice_graves(const int ICELevel, char *graves_p);
// wgraves
	static int ice_wgraves(const int ICELevel, XMLCh *graves_p);
// keyword_end
    static void ice_keyword_end(const int ICELevel, const enum ICEKEY ICEKeywordID, int ice_stack_status);
// keyword_graves_end
    static void ice_keyword_graves_end(const int ICELevel, const enum ICEKEY ICEKeywordID, int ice_stack_status);
// option_level
	static int ice_option_level(const int ICELevel);
// push
	int ice_push(const XMLCh *html_out, const enum ICEKEY ICEKeywordID);
// checking push avoid overloading stack (sets ice_stack_status)
    int ice_push_chk(PSTACK pstack, u_i4 flags, u_i4 level, enum ICEKEY handler, char* data, u_i4 dlen, PICEGENLN* igenln);
// checking flush avoid overloading stack (resets ice_stack_status)
    int ice_flush_chk(PSTACK pstack, OUTHANDLER fnout);
// set_parent_flag
    int ice_set_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry);
// reset_parent_flag
    static int ice_reset_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry);
// test_parent_flag
    static int ice_test_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry);

    // Translator Functions (one per ICE reserved word i.e. Key word or Option word)
// attribute   O(ption)
	int ice_attribute_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_attribute_end(const int ICELevel);
// commit      K(eyword)
	int ice_commit_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_commit_end(const int ICELevel);
// condition      K
	int ice_condition_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_condition_end(const int ICELevel);
// declare      K
	int ice_declare_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_declare_end(const int ICELevel);
// else      K
	int ice_else_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_else_end(const int ICELevel);
// if     K
    int ice_if_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_if_end(const int ICELevel);
// include     K
    int ice_include_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_include_end(const int ICELevel);
// function     K
    int ice_function_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_function_end(const int ICELevel);
// html         O(ption)
    int ice_html_begin(const int ICELevel, const Attributes& attributeList);
	int ice_html_end(const int ICELevel);
// option		O
	int ice_option_begin(const int ICELevel, const enum ICEKEY option, const   Attributes& attributeList);
	int ice_option_end(const int ICELevel, const enum ICEKEY option);
// parameter    O
    int ice_parameter_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_parameter_end(const int ICELevel);
// parent		K
	int ice_parent_begin(const int ICELevel, const enum ICEKEY option, const   Attributes& attributeList);
	int ice_parent_end(const int ICELevel, const enum ICEKEY option);
// query        K
    int ice_query_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_query_end(const int ICELevel);
// relation_display    O
    int ice_relation_display_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_relation_display_end(const int ICELevel);
// rollback     K
	int ice_rollback_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_rollback_end(const int ICELevel);
// sql          O
    int ice_sql_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_sql_end(const int ICELevel);
// statement    O
    int ice_statement_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_statement_end(const int ICELevel);
// then    K
    int ice_then_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_then_end(const int ICELevel);
// var          O
    int ice_var_begin(const int ICELevel, const   Attributes& attributeList);
	int ice_var_end(const int ICELevel);
// UNKNOWN
    int ice_unknown_begin(const int ICELevel, const XMLCh *qname);
	int ice_unknown_end(const int ICELevel, const XMLCh *qname);
};
