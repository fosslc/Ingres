/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: ICETranslateHandlers.cpp
**
** Description:
**    SAX2 Handler functions, invoked by the SAX XML parser, to translate
**    the well formed XML ICE language into the ICE 2.0 language.
**
** Extended Description:
**    This module contains populated SAX2 Handler functions, invoked
**    by the SAX XML parser, required to translate the well
**    formed XML ICE language into the ICE 2.0 language.
**    The main entry points of interest are startElement and endElement.
**    These are called by the SAX parser and decide if the element is an
**    ICE element or not. If an ICE element has been found, it is passed to
**    the approprite method for translation. The nature of the ICE 2.0
**    language means that although there is a 1:1 corespondance
**    between the old and the new keywords we must use a stack
**    structure to cope with the 'REPEAT' modifier and we also need to
**    keep track of where we are in the stack so as to add the correct
**    number of grave quotes ("`"). Accordingly, rather than being able to
**    output translations directly, all elements (both HTML and
**    translated ICE) encountered are first placed on the stack, with
**    the correct number of grave quotes for ICE elements. Only when
**    the level of quotes required is zero and an ICE element has been
**    encountered is it safe to output the stack contents. 
**
**
** History:
**      13-Mar-2001 (peeje01)
**          SIR 103810
**          Created.
**      01-Jun-2001 (peeje01)
**          SIR 103810
**          Changes to move to Solaris
**          Current XERCES version now 1.4, upgraded from 1.3
**          Changes to avoid bug in XMLString::compareNIString
**          Use ice_add_attribute wherever appropriate
**      25-Jul-2001 (peeje01)
**          SIR 103810
**          Add syntax for ICE server functions, XML generataion support:
**          XML, XMLPDATA and SQL output format options XML & XMLPDATA
**      27-Sep-2001 (peeje01)
**          BUG 105899
**          Dynamically allocate storage for character strings. Previously
**          long runs of character data (>256 chars) were being truncated. 
**          This fix exposed problems with the stack code which are also
**          fixed, in this file by only attempting to store information
**          on the stack if previous calls to igen_setlen returned an OK status
**          and in the stack code by correcting space calculations
**      15-Nov-2001 (peeje01)
**          BUG 106385
**          Fix operation of ice_push_chk. Ensure *_all_* parameters are
**          passed through to igen_setgenln.
**      08-Aug-2002 (peeje01)
**          Bug 106445
**          Re-architect i3ce_if syntax to allow for defined and expression
**          tests.
**          Add elements i3ce_conditionExpression, i3ce_conditionDefined
**          There is no support for AND/OR/NOT in this change.
**      12-Mar-2004 (gupsh01)
**          Updated to support Xerces 2.3. 
**	1-Feb-2010 (kschendel for drivi01) B123194
**	    Update writeChars to use INGXMLSize_t for Xerces 3.
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
 * $Log: SAX2PrintHandlers.cpp,v $
 * Revision 1.2  2000/08/09 22:20:38  jpolast
 * updates for changes to sax2 core functionality.
 *
 * Revision 1.1  2000/08/02 19:16:14  jpolast
 * initial checkin of SAX2Print
 *
 *
 */



// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include "ICETranslate.hpp"

#include <compat.h>
#include <me.h>
#include <icestack.h>
#include <icegen.h>
#include <erwu.h>

// ---------------------------------------------------------------------------
//  Local const data
//
//  Note: This is the 'safe' way to do these strings. If you compiler supports
//        L"" style strings, and portability is not a concern, you can use
//        those types constants directly.
// ---------------------------------------------------------------------------
static const XMLCh  gEndElement[] = { chOpenAngle, chForwardSlash, chNull };
static const XMLCh  gEndPI[] = { chQuestion, chCloseAngle, chNull };
static const XMLCh  gStartPI[] = { chOpenAngle, chQuestion, chNull };
static const XMLCh  gXMLDecl1[] =
{
        chOpenAngle, chQuestion, chLatin_x, chLatin_m, chLatin_l
    ,   chSpace, chLatin_v, chLatin_e, chLatin_r, chLatin_s, chLatin_i
    ,   chLatin_o, chLatin_n, chEqual, chDoubleQuote, chDigit_1, chPeriod
    ,   chDigit_0, chDoubleQuote, chSpace, chLatin_e, chLatin_n, chLatin_c
    ,   chLatin_o, chLatin_d, chLatin_i, chLatin_n, chLatin_g, chEqual
    ,   chDoubleQuote, chNull
};

static const XMLCh  gXMLDecl2[] =
{
        chDoubleQuote, chQuestion, chCloseAngle
    ,   chCR, chLF, chNull
};
// peeje01: ICE additions
static const XMLCh  gStartComment[] = { chOpenAngle, chBang, chDash, chDash, chNull };
static const XMLCh  gEndComment[] = { chDash, chDash, chCloseAngle, chNull };
static const XMLCh stOpenAngle[2] = {chOpenAngle, L'\0'};
static const XMLCh stCloseAngle[2] = {chCloseAngle, L'\0'};
static const XMLCh stSpace[2] = {chSpace, L'\0'};
static const XMLCh stEqual[2] = {chEqual, L'\0'};
static const XMLCh stDoubleQuote[2] = {chDoubleQuote, L'\0'};
static const XMLCh stColon[2] = {chColon, L'\0'};


// peeje01: stack additions

XMLCh html_out[MAX_OLD_ICE_STATEMENT_LENGTH];
STKHANDLER  mytable[5] =
    { NULL, NULL, NULL, NULL, NULL };
STATUS status = OK;
// Stack Pointer
PSTACK pstack;
// Current Stack Entry
PICEGENLN igenln;

# ifdef NT_GENERIC
extern "C" int __cdecl printout( char* outstr )
# else
extern int printout( const char* outstr )
# endif
{
    cout << outstr;
    return(0);
}




// ---------------------------------------------------------------------------
//  SAX2ICETranslate: Constructors and Destructor
// ---------------------------------------------------------------------------
SAX2ICETranslate::SAX2ICETranslate( const  char *const encodingName
                                , const XMLFormatter::UnRepFlags unRepFlags
				, const bool expandNamespaces) :


	fFormatter
    (
        encodingName, 0
        , this
        , XMLFormatter::NoEscapes
        , unRepFlags
    ),
	fExpandNS ( expandNamespaces )
{

    STATUS status = OK;
	
    ICELevel = ICE_ROOT_LVL; // 0 is the first level.
    ICENextLevel = 0; // 0 is the first level.
    ICEFlush = ICE_EXTEND_ROOT; // OK to flush the stack, at any time...(reset by i3ce_extend)
    ice_parameter_count = 0; // 0 means this is the first parameter (for functions and includes)
    ice_stack_status = OK; // work around to avoid blowing the stack
	
	// cout << "<!-- Debug: ICE Translator: ICE Level is " << ICELevel << " --> \n";
	// Initialise the Stack 
	// Note: this is a hardwired limit and could lead to problems.
    if ((status = stk_alloc( ICE_STACK_SIZE, &mytable[0], &pstack )) != OK)
    {
       // Throw an exception
	cout << "<!-- ICE Translator: stack allocation failed! -->\n" ;
    }

    //
    //  Go ahead and output an XML Decl with our known encoding. This
    //  is not the best answer, but its the best we can do until we
    //  have SAX2 support.
    //
    // fFormatter << gXMLDecl1 << fFormatter.getEncodingName() << gXMLDecl2;

    XMLString::copyString(html_out, gXMLDecl1);
    XMLString::catString(html_out, fFormatter.getEncodingName()) ;
    XMLString::catString(html_out, gXMLDecl2) ;

    // Push the characters onto the stack
    ice_push(html_out, RAW);

    // Nothing else can have happened, so flush the stack
    ice_flush_chk(pstack, printout);

    ice_push((XMLCh *)L"\n<!-- This has been generated automatically: DO NOT EDIT! -->\n",RAW);
    ice_flush_chk(pstack, printout);
}

SAX2ICETranslate::~SAX2ICETranslate()
{
    // peeje01: stack additions
    // clean up stack memory
    stk_free( pstack );
}


// ---------------------------------------------------------------------------
//  SAX2ICETranslate: Overrides of the output formatter target interface
// ---------------------------------------------------------------------------
void SAX2ICETranslate::writeChars(const XMLByte* const toWrite)
{
    ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
    RAW, (char *)toWrite, strlen((char *)toWrite) +1, &igenln);
}


void SAX2ICETranslate::writeChars(const XMLByte* const toWrite,
                                   INGXMLSize_t count,
                                   XMLFormatter* const formatter)
{
    // For this one, just dump them to the standard output
    // Surprisingly, Solaris was the only platform on which
    // required the char* cast to print out the string correctly.
    // Without the cast, it was printing the pointer value in hex.
    // Quite annoying, considering every other platform printed
    // the string with the explicit cast to char* below.
  cout.write((char *) toWrite, (int) count);
	cout.flush();
}


// ---------------------------------------------------------------------------
//  SAX2ICETranslate: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void SAX2ICETranslate::error(const SAXParseException& e)
{
    cerr << "\nError at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void SAX2ICETranslate::fatalError(const SAXParseException& e)
{
    cerr << "\nFatal Error at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void SAX2ICETranslate::warning(const SAXParseException& e)
{
    cerr << "\nWarning at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}


// ---------------------------------------------------------------------------
//  SAX2ICETranslate: Overrides of the SAX DTDHandler interface
// ---------------------------------------------------------------------------
void SAX2ICETranslate::unparsedEntityDecl(const     XMLCh* const name
                                          , const   XMLCh* const publicId
                                          , const   XMLCh* const systemId
                                          , const   XMLCh* const notationName)
{
    // Not used at this time
}


void SAX2ICETranslate::notationDecl(const   XMLCh* const name
                                    , const XMLCh* const publicId
                                    , const XMLCh* const systemId)
{
    // Not used at this time
}


// ---------------------------------------------------------------------------
//  SAX2ICETranslate: Overrides of the SAX DocumentHandler interface
// ---------------------------------------------------------------------------
void SAX2ICETranslate::characters(const     XMLCh* const    chars,
				const   unsigned int    length)
{
	// Push the characters onto the stack

    char *html_out_ch;
    STATUS status;

    if ((html_out_ch = MEreqmem(0, length+1, TRUE, NULL)) != NULL)
    {
	strncpy(html_out_ch, StrX(chars).localForm(), length);

	status = 
	ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		RAW, html_out_ch, length+1, &igenln);
	MEfree(html_out_ch);

	if (status != OK) {
		// Most likely cause of failure is stack overflow.
		// Issue a warning in to the output stream (safer for batch)
            cout << "\nWarning: Input too large, truncated\n";
	}
    }
}


void SAX2ICETranslate::endDocument()
{

	// Must flush the stack because we are ending
	ice_flush_chk(pstack, printout);
}


void SAX2ICETranslate::endElement(const XMLCh* const uri, 
								 const XMLCh* const localname, 
								 const XMLCh* const qname)
{
	if (strncmp("i3ce_", StrX(qname).localForm(), 5)) {
		// No i3ce keyword found. Normal processing.
		    // No escapes are legal here
		// fFormatter << XMLFormatter::NoEscapes << gEndElement ;
		XMLString::copyString(html_out, gEndElement);
		if ( fExpandNS )
		{
			if (XMLString::compareIString(uri,XMLUni::fgEmptyString) != 0) {
					// fFormatter  << uri << chColon;
					XMLString::catString(html_out, stColon);
			}
			// fFormatter << localname << chCloseAngle;
			XMLString::catString(html_out, localname);
			XMLString::catString(html_out, stCloseAngle);
		}
		else {
			// fFormatter << qname << chCloseAngle;
			XMLString::catString(html_out, qname);
			XMLString::catString(html_out, stCloseAngle);
		}

	    // Push the constructed end tag onto the stack
		ice_push(html_out, RAW);

	} else {
		// We have found an i3ce keyword
		    // No escapes are legal here
		// fFormatter << XMLFormatter::NoEscapes;

		// Which keyword?
		char keyword[256];
		char *keyword_p = keyword;
		int keyword_idx = 0;

		// copy the name to a local string
		strncpy(keyword, StrX(qname).localForm(),255);

		// fixme: skip over the "i3ce_" bit (what about namespaces????)
		keyword_p+=5;

        
        // attribute -end
        if (0 == strncmp("attribute", keyword_p, 9))
        {
			ice_attribute_end(ICELevel);
		}
        // case -end
        else if (0 == strncmp("case", keyword_p, 4))
        {
			    ice_option_end(ICELevel, CASE);
		}
        // commit -end
        else if (0 == strncmp("commit", keyword_p, 6))
        {
			ice_commit_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1; 
		}
	// conditionExpression - end
        else if(0 == strncmp("conditionExpression ", keyword_p, 19))
        {
			ice_option_end(ICELevel, CONDITIONeXPRESSION);
	}
	// conditionDefined - end
        else if(0 == strncmp("conditionDefined", keyword_p, 15))
        {
			ice_option_end(ICELevel, CONDITIONdEFINED);
	}
	// condition - end
        else if(0 == strncmp("condition", keyword_p, 9))
        {
			// ice_condition_end(ICELevel);
	    ice_option_end(ICELevel, CONDITION);
	}
		// declare - end
        else if(0 == strncmp("declare", keyword_p, 7))
        {
			ice_declare_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		} 
        // default -end
        else if (0 == strncmp("default", keyword_p, 7))
        {
			    ice_option_end(ICELevel, DEFAULT);
		}
		// description -end
        else if (0 == strncmp("description", keyword_p, 11))
        {
			    ice_option_end(ICELevel, DESCRIPTION);
		}
        // else -end
        else if (0 == strncmp("else", keyword_p, 4))
        {
            ice_else_end(ICELevel);
		}
		// AttributeName -end
        else if (0 == strncmp("AttributeName", keyword_p, 13))
        {
			    ice_option_end(ICELevel, EaTTRIBUTEnAME);
		}
		// AttributeValue -end
        else if (0 == strncmp("AttributeValue", keyword_p, 14))
        {
			    ice_option_end(ICELevel, EaTTRIBUTEvALUE);
		}
		// Attributes -end
        else if (0 == strncmp("Attributes", keyword_p, 10))
        {
			    ice_option_end(ICELevel, EaTTRIBUTES);
		}
		// Attribute -end
        else if (0 == strncmp("Attribute", keyword_p, 9))
        {
			    ice_option_end(ICELevel, EaTTRIBUTE);
		}
		// Children -end
        else if (0 == strncmp("Children", keyword_p, 8))
        {
			    ice_option_end(ICELevel, EcHILDREN);
		}
		// Child -end
        else if (0 == strncmp("Child", keyword_p, 5))
        {
			    ice_option_end(ICELevel, EcHILD);
		}
		// extend -end
        else if (0 == strncmp("extend", keyword_p, 6))
        {
			    ice_option_end(ICELevel, EXTEND);
		}
		// ext -end
        else if (0 == strncmp("ext", keyword_p, 3))
        {
			    ice_option_end(ICELevel, EXT);
		}
		// function -end
        else if (0 == strncmp("function", keyword_p, 8))
        {
            ice_function_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		}
		// headers -end
        else if (0 == strncmp("headers", keyword_p, 7))
        {
            ice_option_end(ICELevel, HEADERS);
		}
		// headers -end
        else if (0 == strncmp("header", keyword_p, 6))
        {
            ice_option_end(ICELevel, HEADER);
		}
        // html -end
        else if (0 == strncmp("html", keyword_p, 4))
        {
            ice_option_end(ICELevel, HTML);
		}
        // hyperlink -end
        else if (0 == strncmp("hyperlink", keyword_p, 9))
        {
            ice_option_end(ICELevel, HYPERLINK);
		}
        // hyperlinkAttributes -end
        else if (0 == strncmp("hyperlinkAttributes", keyword_p, 19))
        {
            ice_option_end(ICELevel, HYPERLINKaTTRS);
		}
        // if -end
        else if (0 == strncmp("if", keyword_p, 2))
        {
            ice_if_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		}
        // include -end
        else if (0 == strncmp("include", keyword_p, 8))
        {
            ice_include_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		} 
        // links -end
        else if (0 == strncmp("links", keyword_p, 5))
        {
			    ice_option_end(ICELevel, LINKS);
		} 
        // link -end
        else if (0 == strncmp("link", keyword_p, 4))
        {
			    ice_option_end(ICELevel, LINK);
		}
        // nullvar -end
        else if (0 == strncmp("nullvar", keyword_p, 7))
        {
			    ice_option_end(ICELevel, NULLVAR);
		} 
        // parameters -end
        else if (0 == strncmp("parameters", keyword_p, 10))
        {
			    ice_option_end(ICELevel, PARAMETERS);
		} 
        // parameter -end
        else if (0 == strncmp("parameter", keyword_p, 9))
        {
			    ice_option_end(ICELevel, PARAMETER);
		}
        // query -end
        else if (0 == strncmp("query", keyword_p, 5))
        {
            ice_query_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		} 
        // relation_display -end
        else if (0 == strncmp("relation_display", keyword_p, 16))
        {
			    ice_relation_display_end(ICELevel);
		}
        // rollback -end
        else if (0 == strncmp("rollback", keyword_p, 8))
        {
			ice_rollback_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1; 
		} 
        // rows -end
        else if (0 == strncmp("rows", keyword_p, 4))
        {
			    ice_option_end(ICELevel, ROWS);
		}
        // sql -end
        else if (0 == strncmp("sql", keyword_p, 3))
        {
			    ice_sql_end(ICELevel);
		} 
        // statement -end
        else if (0 == strncmp("statement", keyword_p, 9))
        {
			    ice_statement_end(ICELevel);
	}
	// switch -end
        else if (0 == strncmp("switch", keyword_p, 6))
        {
	    ice_parent_end(ICELevel, SWITCH);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1; 
	}
	// system -end
        else if (0 == strncmp("system", keyword_p, 6))
        {
	    ice_parent_end(ICELevel, SYSTEM);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
		}
        // target -end
        else if (0 == strncmp("target", keyword_p, 6))
        {
            ice_option_end(ICELevel, TARGET);
	} 
        // then -end
        else if (0 == strncmp("then", keyword_p, 4))
        {
            ice_then_end(ICELevel);
	} 
        // var -end
        else if (0 == strncmp("var", keyword_p, 3))
        {
            ice_var_end(ICELevel);
            // ICE Keyword: bump down the ICE level
            ICENextLevel = ICELevel;
            ICELevel = (ICELevel == 0) ? 0 : ICELevel-= 1;
	}
        // xmlpdata -end
        else if (0 == strncmp("xmlpdata", keyword_p, 8))
        {
            ice_option_end(ICELevel, XMLPDATA);
		}
        // xml -end
        else if (0 == strncmp("xml", keyword_p, 3))
        {
            ice_option_end(ICELevel, XML);
		}
        // unknown -end
        else
        {
            ice_unknown_end(ICELevel, qname);
        }
	// ice_push(L" ", RAW);
	if (ICENextLevel == ICE_ROOT_LVL && ICEFlush == ICE_EXTEND_ROOT) {
	    // Must flush the stack because we are just entering the first level ICE 
	    ice_flush_chk(pstack, printout);
	} // root level
    } // i3ce name
} // end_element



void SAX2ICETranslate::ignorableWhitespace( const   XMLCh* const chars
                                            ,const  unsigned int length)
{
  	
    // Ignorable white space occurs 'outside' tags
	// For ICE 2.0 this white space must be removed
	
}


void SAX2ICETranslate::processingInstruction(const  XMLCh* const target
                                            , const XMLCh* const data)
{
    fFormatter << XMLFormatter::NoEscapes << gStartPI  << target;
    if (data)
        fFormatter << chSpace << data;
    fFormatter << XMLFormatter::NoEscapes << gEndPI;
}


void SAX2ICETranslate::startDocument()
{
}


void SAX2ICETranslate::startElement(const   XMLCh* const    uri,
									const   XMLCh* const    localname,
									const   XMLCh* const    qname,
                                    const   Attributes&		attributes)
{

    // The name has to be representable without any escapes
    if (strncmp("i3ce_", StrX(qname).localForm(), 5))
    {
	// No i3ce keyword found. Normal processing.
	html_out[0]=chOpenAngle;
	html_out[1]=L'\0';

	if ( fExpandNS )
	{
	    if (XMLString::compareIString(uri,XMLUni::fgEmptyString) != 0) {
		XMLString::catString(html_out, uri) ;
		XMLString::catString(html_out, stColon ) ;
	    }
	    XMLString::catString(html_out, localname ) ;
	} else {
	    XMLString::catString(html_out, qname) ;
	}

	unsigned int len = attributes.getLength();
	for (unsigned int index = 0; index < len; index++)
	{
	    //
	    //  Again the name has to be completely representable. But the
	    //  attribute can have refs and requires the attribute style
	    //  escaping.
	    //
	    // fFormatter  << XMLFormatter::NoEscapes << chSpace ;
	    XMLString::catString(html_out, stSpace ) ;
	    if ( fExpandNS )
	    {
		if (XMLString::compareIString(attributes.getURI(index),XMLUni::fgEmptyString) != 0)
		{
		    XMLString::catString(html_out, stColon ) ;
		}
		XMLString::catString(html_out, attributes.getLocalName(index) ) ;
	    }
	    else
	    {
		XMLString::catString(html_out, attributes.getQName(index) ) ;
	    }
	    XMLString::catString(html_out, stEqual);
	    XMLString::catString(html_out, stDoubleQuote);
	    XMLString::catString(html_out, attributes.getValue(index));
	    XMLString::catString(html_out, stDoubleQuote);
	}
	XMLString::catString(html_out, stCloseAngle);

	// Push the tag onto the stack in local form
	char *html_out_chp ;

	html_out_chp = XMLString::transcode(html_out);

	ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	    RAW, html_out_chp, strlen(html_out_chp) + 1, &igenln);
	free(html_out_chp);

    }
    else
    {
	// We have found an i3ce keyword
	if (ICENextLevel == ICE_ROOT_LVL && ICEFlush == ICE_EXTEND_ROOT)
	{
	    // Must flush the stack because we are just
	    // entering the first level ICE 
	    // cout << "<!-- Debug: Flushing stack! (ICE Level is: " << ICELevel << ") -->\n" ;
	    ice_flush_chk(pstack, printout);
	    // cout << "<!-- Debug: Stack flushed! -->\n" ;
	}
	// DEBUG: ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY, ICELevel, ICEOPEN, "<!-- SAX ICE startElement -->\n",31, &igenln);
	    
	// Which keyword?
	char keyword[256];
	char *keyword_p = keyword;
	int keyword_idx = 0;

	// copy the name to a local string
	strncpy(keyword, StrX(qname).localForm(),255);

	// fixme: skip over the "i3ce_" bit (what about namespaces????)
	keyword_p+=5;

	// attribute
	if (0 == strncmp("attribute", keyword_p, 9))
	{ 
	    ice_attribute_begin(ICELevel, attributes);
	}
	// case
	else if (0 == strncmp("case", keyword_p, 4))
	{
	    ice_option_begin(ICELevel, CASE, attributes);
	}
	// commit
	else if (0 == strncmp("commit", keyword_p, 6))
	{ 
	     // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_commit_begin(ICELevel, attributes);
	}
	// begin i3ce_conditionExpression
	else if (0 == strncmp("conditionExpression", keyword_p, 19))
	{
	    ice_option_begin(ICELevel, CONDITIONeXPRESSION, attributes);
	}
	// begin i3ce_conditionDefined
	else if (0 == strncmp("conditionDefined", keyword_p, 15))
	{
	    ice_option_begin(ICELevel, CONDITIONdEFINED, attributes);
	}
	// condition
	else if (0 == strncmp("condition", keyword_p, 9))
	{ 
	    // ice_condition_begin(ICELevel , attributes);
	    ice_option_begin(ICELevel, CONDITION, attributes);
	}
	// declare
	else if (0 == strncmp("declare", keyword_p, 7))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++; 
	    ice_declare_begin(ICELevel, attributes);
	}
	// default
	else if (0 == strncmp("default", keyword_p, 7))
	{
	    ice_option_begin(ICELevel, DEFAULT, attributes);
	}
	// description
	else if (0 == strncmp("description", keyword_p, 11))
	{
	    ice_option_begin(ICELevel, DESCRIPTION, attributes);
	}
	// AttributeName
	else if (0 == strncmp("AttributeName", keyword_p, 13))
	{
	    ice_option_begin(ICELevel, EaTTRIBUTEnAME, attributes);
	}
	// AttributeValue
	else if (0 == strncmp("AttributeValue", keyword_p, 14))
	{
	    ice_option_begin(ICELevel, EaTTRIBUTEvALUE, attributes);
	}
	// Attributes
	else if (0 == strncmp("Attributes", keyword_p, 10))
	{
	    ice_option_begin(ICELevel, EaTTRIBUTES, attributes);
	}
	// Attribute
	else if (0 == strncmp("Attribute", keyword_p, 9))
	{
	    ice_option_begin(ICELevel, EaTTRIBUTE, attributes);
	}
	// Children
	else if (0 == strncmp("Children", keyword_p, 8))
	{
	    ice_option_begin(ICELevel, EcHILDREN, attributes);
	}
	// Child
	else if (0 == strncmp("Child", keyword_p, 5))
	{
	    ice_option_begin(ICELevel, EcHILD, attributes);
	}
	// Description
	else if (0 == strncmp("description", keyword_p, 5))
	{
	    ice_option_begin(ICELevel, EcHILD, attributes);
	}
	// else
	else if (0 == strncmp("else", keyword_p, 4))
	{
	    ice_else_begin(ICELevel, attributes);
	}
	// extend
	else if (0 == strncmp("extend", keyword_p, 6))
	{
	    ice_option_begin(ICELevel, EXTEND, attributes);
	}
	// ext
	else if (0 == strncmp("ext", keyword_p, 3))
	{
	    ice_option_begin(ICELevel, EXT, attributes);
	}
	// function
	else if (0 == strncmp("function", keyword_p, 8))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_function_begin(ICELevel, attributes);
	}
	// headers
	else if (0 == strncmp("headers", keyword_p, 7))
	{
	    ice_option_begin(ICELevel, HEADERS, attributes);
	}
	// header
	else if (0 == strncmp("header", keyword_p, 6))
	{
	    ice_option_begin(ICELevel, HEADER, attributes);
	}
	// html
	else if (0 == strncmp("html", keyword_p, 4))
	{
	    ice_option_begin(ICELevel, HTML, attributes);
	}
	// hyperlink
	else if (0 == strncmp("hyperlink", keyword_p, 9))
	{
	    ice_option_begin(ICELevel, HYPERLINK, attributes);
	}
	// hyperlinkAttributes
	else if (0 == strncmp("hyperlinkAttributes", keyword_p, 19))
	{
	    ice_option_begin(ICELevel, HYPERLINKaTTRS, attributes);
	}
	// if
	else if (0 == strncmp("if", keyword_p, 2))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_if_begin(ICELevel, attributes);
	}
	// include
	else if (0 == strncmp("include", keyword_p, 7))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_include_begin(ICELevel, attributes);
	}
	// links
	else if (0 == strncmp("links", keyword_p, 5))
	{
	    ice_option_begin(ICELevel, LINKS, attributes);
	}
	// link
	else if (0 == strncmp("link", keyword_p, 4))
	{
	    ice_option_begin(ICELevel, LINK, attributes);
	}
	// nullvar
	else if (0 == strncmp("nullvar", keyword_p, 7))
	{
	    ice_option_begin(ICELevel, NULLVAR, attributes);
	}
	// parameters
	else if (0 == strncmp("parameters", keyword_p, 10))
	{
	    ice_option_begin(ICELevel, PARAMETERS, attributes);
	}
	// parameter
	else if (0 == strncmp("parameter", keyword_p, 9))
	{
	    ice_option_begin(ICELevel, PARAMETER, attributes);
	}
	// query
	else if (0 == strncmp("query", keyword_p, 5))
	{
	     // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_query_begin(ICELevel, attributes);
	}
	// relation_display
	else if (0 == strncmp("relation_display", keyword_p, 16))
	{
	    ice_relation_display_begin(ICELevel, attributes);
	}
	// rollback
	else if (0 == strncmp("rollback", keyword_p, 8))
	{ 
	     // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_rollback_begin(ICELevel, attributes);
	}
	// rows
	else if (0 == strncmp("rows", keyword_p, 4))
	{
	    ice_option_begin(ICELevel, ROWS, attributes);
	}
	// sql
	else if (0 == strncmp("sql", keyword_p, 3))
	{
	    ice_sql_begin(ICELevel, attributes);
	}
	// statement
	else if (0 == strncmp("statement", keyword_p, 9))
	{
	    ice_statement_begin(ICELevel, attributes);
	}
	// switch
	else if (0 == strncmp("switch", keyword_p, 6))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_parent_begin(ICELevel, SWITCH, attributes);
	}
	// system
	else if (0 == strncmp("system", keyword_p, 8))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_parent_begin(ICELevel, SYSTEM, attributes);
	}
	// target
	else if (0 == strncmp("target", keyword_p, 6))
	{
	    ice_option_begin(ICELevel, TARGET, attributes);
	}
	// then
	else if (0 == strncmp("then", keyword_p, 4))
	{
	    ice_then_begin(ICELevel, attributes);
	}
	// var
	else if (0 == strncmp("var", keyword_p, 3))
	{
	    // ICE Keyword: bump up the ICE level
	    ICELevel=ICENextLevel;
	    ICENextLevel++;
	    ice_var_begin(ICELevel, attributes);
	}
	// xmlpdata
	else if (0 == strncmp("xmlpdata", keyword_p, 8))
	{
	    ice_option_begin(ICELevel, XMLPDATA, attributes);
	}
	// xml
	else if (0 == strncmp("xml", keyword_p, 3))
	{
	    ice_option_begin(ICELevel, XML, attributes);
	}
	// unknown
	else
	{
	    ice_unknown_begin(ICELevel, qname);
	}
    }

    unsigned int len = attributes.getLength();
    for (unsigned int index = 0; index < len; index++)
    {
	//
	//  Again the name has to be completely representable. But the
	//  attribute can have refs and requires the attribute style
	//  escaping.
	//
	if ( fExpandNS )
	{
	    if (XMLString::compareIString(attributes.getURI(index),XMLUni::fgEmptyString) != 0)
	    {
		// fFormatter  << attributes.getURI(index) << chColon;
		;
	    }
	    // fFormatter  << attributes.getLocalName(index) ;
	    ;
	}
	else
	{
	    // fFormatter << attributes.getValue(index);
	}
    }
}


/* {
** Name: ice_add_attribute
**
** Description:
**      Add a named XML attribute to a string
**
** Inputs:
**      attributeName      Pointer to the wide string attribute Name
**      attributeList      Address of the structure containing the
**                            attributes
**
** Outputs:
**      oldICE_p    Pointer to the returned string.
**
** Return:
**      OK  Command completed successfully.
**      -1  The attribute requested was not found.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline int SAX2ICETranslate::ice_add_attribute(char *oldICE_p, const wchar_t *const attributeName, const Attributes& attributeList)
{
    int retval = -1;
    char *attributeString_p;

    retval = attributeList.getIndex((XMLCh *)attributeName);
    if (retval != -1)
    {
	// process name attribute
	attributeString_p = XMLString::transcode(attributeList.getValue(retval));
	strcat(oldICE_p, attributeString_p);
	free(attributeString_p);
	retval = OK;
    }
    return (retval);

} // ice_add_attribute }

/* {
** Name: ice_power2
**
** Description:
**      Calculates 2 raised to the power n
**
** Inputs:
**      n    the power to raise 2 to
**
** Outputs:
**      None
**
** Return:
**      2**n
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline int SAX2ICETranslate::ice_power2(const int n) {
	int l, x;

	l = n;
	
	for(x = 1; l > 0; --l)
		x = x * 2;

	return(x);
} // ice_power2 }

/* {
** Name: ice_graves
**
** Description:
**      Given a nesting level, add the correct number of graves to a string
**      It is the responsability of the caller to ensure that the
**      string variable is large enough.
**
** Inputs:
**      ICELevel    Integer value of the nesting level
**
** Outputs:
**      graves_p    Pointer to a string that will contain the graves
**
** Return:
**      OK                      Command completed successfully.
**
** History:
**      08-Feb-2001 (fanra01)
**          Created.
*/
inline int SAX2ICETranslate::ice_graves(const int ICELevel, char *graves_p) {
	int grave_number;
		    
    grave_number = SAX2ICETranslate::ice_power2(ICELevel);

	for(int l = 0; l < grave_number; graves_p++, l++)
	    *graves_p = chGrave;

	*graves_p='\0';

	return(OK);
} // ice_graves }

/* {
** Name: ice_wgraves
**
** Description:
**      Given a nesting level, add the correct number of graves to a string
**      It is the responsability of the caller to ensure that the
**      string variable is large enough.
**
** Inputs:
**      ICELevel    Integer value of the nesting level
**
** Outputs:
**      wgraves_p    Pointer to a wide string that will contain the graves
**
** Return:
**      OK                      Command completed successfully.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline int SAX2ICETranslate::ice_wgraves(const int ICELevel, XMLCh *wgraves_p) {
	int grave_number;
		    
    grave_number = SAX2ICETranslate::ice_power2(ICELevel);

	for(int l = 0; l < grave_number; wgraves_p++, l++)
	    *wgraves_p = chGrave;

	*wgraves_p=L'\0';

	return(OK);
} // ice_wgraves }

// Parent Flag Methods {
/* {
** Name: ice_set_parent_flag
**
** Description:
**      Sets requested flag in the parent element
**
** Inputs:
**      Flag                    The flag to be set
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      -1			An error was encountered.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int ice_set_parent_flag(const int Flag, const PICEGENLN CurrentStackEntry) {

    int retval = 1;
    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        retval = igen_setflags(parent, Flag) ; 
    }

    return(retval);
} // ice_set_parent_flag }


/* {
** Name: reset_parent_flag
**
** Description:
**      Resets requested flag in the parent element
**
** Inputs:
**      Flag                    The flag to be reset
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int ice_reset_parent_flag(const int Flag, const PICEGENLN CurrentStackEntry) {

    int retval = FAIL;
    u_i4 resetFlag;

    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        // Invert the flag setting to reset the flag
        resetFlag = ~Flag;
        retval = igen_setflags(parent, resetFlag) ; 
    }

    return (retval);

} // ice_reset_parent_flag }

/* {
** Name:  test_parent_flag
**
** Description:
**      Tests requested flag in the parent element
**
** Inputs:
**      Flag                    The flag to be reset
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      Value of the flag being tested
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int ice_test_parent_flag(const int Flag, const PICEGENLN CurrentStackEntry) {
    int retval = 1;
 
    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        retval = igen_testflags(parent, Flag) ; 
    }
    else
    {
        cout << "ice_test_parent_flag couldn't get parent" ;
    }

    return(retval);

} // ice_test_parent_flag }
// Parent Flag Methods }


/* {
** Name: ice_option_level
**
** Description:
**      Calculates the adjusted level for ICE options
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      The adjusted level
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline int SAX2ICETranslate::ice_option_level(const int ICELevel) {
    // return ((ICELevel == 0) ? 0 : ICELevel -1);
    return(ICELevel);
} // ice_option_level }

/* {
** Name: ice_keyword_end
**
** Description:
**      Terminates an old ice keyword correctly. The termination
**      sequence is pushed onto the stack.
**
** Inputs:
**      ICELevel        The ICE nesting level
**      ICEKeywordID    The ID of the ICE keyword
**      ice_stack_status Only update the stack if set to OK
**
** Outputs:
**      ice_stack_status set by call to igen_setgenln (normally OK)
**
** Return:
**      void
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline void SAX2ICETranslate::ice_keyword_end(const int ICELevel, const enum ICEKEY ICEKeywordID, int ice_stack_status) {
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;

    // Terminate the tag
    strcpy(old_ice_p, " -->\n");
	
    // Push old_ice onto the stack
    if(ice_stack_status == OK)
    {
	ice_stack_status = 
	    igen_setgenln( pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	    ICEKeywordID, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
    }
    switch(ice_stack_status)
    {

    case E_STK_NO_ENTRY:
	ice_stack_status = OK;
	break;

    } /* switch (ice_stack_status) */


} // ice_keyword_end }

/* {
** Name: ice_keyword_graves_end
**
** Description:
**      Terminates an old ice keyword correctly. Add the correct
**      number of grave quotes. The termination sequence is pushed
**      onto the stack.
**
** Inputs:
**      ICELevel        The ICE nesting level
**      ICEKeywordID    The ID of the ICE keyword
**      ice_stack_status Only update the stack if set to OK
**
** Outputs:
**      ice_stack_status set by call to igen_setgenln (normally OK)
**
** Return:
**      void
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
void inline SAX2ICETranslate::ice_keyword_graves_end(const int ICELevel, const enum ICEKEY ICEKeywordID, int ice_stack_status) {
	char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
	char *old_ice_p = old_ice;
    char graves[256];
    char *graves_p = graves;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcpy(old_ice_p, graves);
    
    // Say we've terminated the graves
    ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);
    
    // Terminate the tag
    strcat(old_ice_p, " -->\n");

    // Push old_ice onto the stack
    if(ice_stack_status == OK)
    {
	ice_stack_status = 
	    igen_setgenln( pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
		ICEKeywordID, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
    }
    switch(ice_stack_status)
    {

    case E_STK_NO_ENTRY:
	ice_stack_status = OK;
	break;

    } /* switch (ice_stack_status) */


} // ice_keyword_graves_end }

/* {
** Name: attributecp
**
** Description:
**      Concatenate the attribute value to the target
**      Depreciated
**
** Inputs:
**      attribute_value       wide string value of the attribute
**
** Outputs:
**      target                target to concatenate the value to
**
** Return:
**      As strcat()
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
inline char * SAX2ICETranslate::attributecp(char *target, XMLCh *attribute_value){

	// Temp variable so we can free the transcode memory
	register char *attributeString_p;
	char *retval;
	
	attributeString_p = XMLString::transcode(attribute_value);
	retval = strcat(target, attributeString_p);
	free(attributeString_p);

	return(retval);
} // attributecp }


/* {
** Name: ice_push
**
** Description:
**      Pushes an HTML wide string onto the stack
**
** Inputs:
**      html_out           Pointer to the wide string to be pushed
**      ICEKeywordID       ID of the ICE keyword
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_push(const XMLCh *html_out, const enum ICEKEY ICEKeywordID) {
	
    STATUS retval = 0;
    char html_out_ch[256] ;

    for (int i = 0; i< 256; i++) html_out_ch[i]='\0';


    strncpy(html_out_ch, StrX((XMLCh *)html_out).localForm(), 254);
    html_out_ch[255]='\0';
    
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	ICEKeywordID, html_out_ch, strlen(html_out_ch) + 1, &igenln);

    return(retval);
} // ice_push }

/* {
** Name: ice_push_chk
**
** Description:
**      Pushes a string onto the stack, only if there is room. Checks
**      the flag ice_stack_full. Wraps igen_setgenln. Only calls it if
**      igen_setgenln returned a OK status before.
**
** Inputs:
**      pstack      Pointer to the stack to insert into.  Returned from
**                  call to stk_alloc.
**      flags       Flag to determine if the entry is active or if there
**                  are child macros.
**      level       Level of the macro.
**      handler     The ice keyword or option.
**      data        Pointer to the generated ice string.
**      data_length Length of the ice string.
**
**      ice_stack_status (class global) Only update the stack if set to OK
**
** Outputs:
**      None
**
** Side Effects:
**      Sets the value of ice_stack_status
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      29-Oct-2001 (peeje01)
**          Created.
**      15-Nov-2001 (peeje01)
**          BUG 106385
**          Fix operation of ice_push_chk. Ensure *_all_* parameters are
**          passed through to igen_setgenln.
*/
int SAX2ICETranslate::ice_push_chk(PSTACK pstack, u_i4 flags, u_i4 level, enum ICEKEY handler, char* data, u_i4 data_length, PICEGENLN* igenln)
{
    if(ice_stack_status == OK)
    {
	ice_stack_status = 
	igen_setgenln(pstack, flags, level, handler, data, data_length, igenln);
    }
    switch(ice_stack_status)
    {

    case E_STK_NO_ENTRY:
	ice_stack_status = OK;
	break;

    } /* switch (ice_stack_status) */

    return(ice_stack_status);
} // ice_push_chk }

/* {
** Name: ice_flush_chk
**
** Description:
**      Flush the stack. Reset the ice_stack_status.
**
** Inputs:
**      html_out           Pointer to the string to be pushed
**      ICEKeywordID       ID of the ICE keyword
**
** Outputs:
**      None
**
** Side Effects:
**      Sets the value of ice_stack_status
**     
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      29-Oct-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_flush_chk(PSTACK pstack, OUTHANDLER fnout)
{
    STATUS retval = OK;
    
    retval = igen_flush(pstack, fnout);
    ice_stack_status = retval;

    switch(ice_stack_status)
    {

    case E_STK_NO_ENTRY:
	ice_stack_status = OK;
	break;

    } /* switch (ice_stack_status) */


    return(retval);
} // ice_flush_chk }

// Parent Flag Methods {

/* {
** Name: set_parent_flags
**
** Description:
**      Set requested flags in the parent element
**
** Inputs:
**      Flag                    The flag to be set
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      -1			An error was encountered.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_set_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry) {

    int retval = 1;
    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        retval = igen_setflags(parent, Flag) ; 
    }

    return(retval);
} // ice_set_parent_flags }

/* {
** Name: reset_parent_flags
**
** Description:
**      Reset requested flags in the parent element
**
** Inputs:
**      Flag                    The flag to be reset
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/int SAX2ICETranslate::ice_reset_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry) {

    int retval = 1;
    u_i4 resetFlag;

    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        resetFlag = Flag;
        retval = igen_clrflags(parent, resetFlag) ; 
    }

    return (retval);

} // ice_reset_parent_flags }

/* {
** Name:  test_parent_flags
**
** Description:
**      Tests requested flag in the parent element
**
** Inputs:
**      Flag                    The flag to be reset
**      CurrentStackEntry       A pointer to the current stack entry
**
** Outputs:
**      None
**
** Return:
**      Value of the flag being tested
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_test_parent_flags(const int Flag, const PICEGENLN CurrentStackEntry) {
    STATUS retval = FAIL;
 
    PICEGENLN parent;

    retval = igen_getparent(CurrentStackEntry, &parent);
    if( retval == OK)
    {
        retval = igen_testflags(parent, Flag) ; 
    }

    return(retval);

} // ice_test_parent_flags
// Parent Flag Methods }

// ICE Translator Functions (called from the event handlers)


// ice_attribute {
/* {
** Name: ice_attribute_begin
**
** Description:
**      Push the correct ICE attribute begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_attribute_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;
    char graves[256];
    char *graves_p = graves;

    int retval = 0;

    // Construct the tag
    strcpy(old_ice_p, "\n    ATTR=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);


    // Push old_ice onto the stack
    ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
    	COMMIT, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    return(retval);
} // ice_attribute_begin }

/* {
** Name: ice_attribute_end
**
** Description:
**      Push the correct ICE attribute end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_attribute_end(const int ICELevel) {

    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;


    // Terminate an old ICE option correctly
    char graves[256];
    char *graves_p = graves;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcpy(old_ice_p, graves);
    
    // Push old_ice onto the stack
    retval=ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	COMMIT, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    // Say we've terminated the graves
    ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);
    
    return(retval);
} // ice_attribute_end }
// ice_attribute }

// ice_commit {
/* {
** Name: ice_commit_begin
**
** Description:
**      Push the correct ICE commit begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_commit_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    // char *new_ice = keyword_p;
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    int retval = 0;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    strcpy(old_ice_p, "COMMIT=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // process scope attribute & add to tag
    ice_add_attribute(old_ice_p, L"i3ce_transaction", attributeList);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
    	ICELevel, COMMIT, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_commit_begin }

/* {
** Name: ice_commit_end
**
** Description:
**      Push the correct ICE commit end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_commit_end(const int ICELevel) {
    int retval = 0;

    // Terminate an old ICE keyword correctly
    ice_keyword_graves_end(ICELevel, COMMIT, ice_stack_status);

    return(retval);

} // ice_commit_end }
// ice_commit }


// ice_condition {
/* {
** Name: ice_condition_begin
**
** Description:
**      Push the correct ICE condition begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_condition_begin(const int ICELevel, const Attributes&	attributeList) {
    int retval = 0;

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;
    char *attributeString_p;
 
    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Construct the option

    // Add the graves to the tag
    strcpy(old_ice_p, graves);

    // Work out what the condition is
    // process Left Hand Side attribute & add to tag
    attributeString_p = XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_condlhs"));
    strcat(old_ice_p, attributeString_p);
    free(attributeString_p);

    // process operator
    attributeString_p = XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_condop"));
    strcat(old_ice_p, attributeString_p);
    free(attributeString_p);

    // process Right Hand Side attribute & add to tag
    attributeString_p = XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_condrhs"));
    strcat(old_ice_p, attributeString_p);
    free(attributeString_p);


    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ice_option_level(ICELevel), IF, old_ice_p,
	STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_condition_begin }

/* {
** Name: ice_condition_end
**
** Description:
**      Push the correct ICE condition end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_condition_end(const int ICELevel) {

    int retval = 0;
    // Terminate an old ICE option correctly
		
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;
    
    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Add the graves to the tag
    strcpy(old_ice_p, graves);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ice_option_level(ICELevel), CONDITION, old_ice_p,
	STLENGTH( old_ice_p ) + 1, &igenln );
        
    return(retval);

} // ice_condition_end }
// ice_condition }


// ice_declare {
/* {
** Name: ice_declare_begin
**
** Description:
**      Push the correct ICE declare begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_declare_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    // char *new_ice = keyword_p;
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    int retval = 0;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
        ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    strcpy(old_ice_p, "DECLARE=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // process scope attribute & add to tag
    ice_add_attribute(old_ice_p, L"i3ce_scope", attributeList);
    
    strcat(old_ice_p, ".");
    
    // process name attribute
    ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

    strcat(old_ice_p, "=");
    
    // process value attribute
    ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ICELevel, DECLARE, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    return(retval);
} // ice_declare_begin }


/* {
** Name: ice_declare_end
**
** Description:
**      Push the correct ICE declare end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_declare_end(const int ICELevel) {

    int retval = 0;

    // Terminate an old ICE keyword correctly
    ice_keyword_graves_end(ICELevel, DECLARE, ice_stack_status);

    return(retval);

} // ice_declare_end }
// ice_declare }

// ice_else {
/* {
** Name: ice_else_begin
**
** Description:
**      Push the correct ICE else begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_else_begin(const int ICELevel, const   Attributes& attributeList) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    // char *new_ice = keyword_p;
    char *old_ice_p = old_ice;
    char *graves_p = graves;


    // Construct the tag
    strcpy(old_ice_p, "\nELSE=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);
	
    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ice_option_level(ICELevel), IF, old_ice_p,
	STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);

} // ice_else_begin }

/* {
** Name: ice_else_end
**
** Description:
**      Push the correct ICE else end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_else_end(const int ICELevel) {

    int retval = 0;

    // Terminate an old ICE keyword correctly
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;


    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Add the graves to the tag
    strcpy(old_ice_p, graves);
    strcat(old_ice_p, "\n");
    
    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ice_option_level(ICELevel), ELSE, old_ice_p,
	STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);

} // ice_else_end }
// ice_else }

// ice_function {
/* {
** Name: ice_function_begin
**
** Description:
**      Push the correct ICE function begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_function_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    ice_parameter_count = 0; // So far we've had no parameters for this function

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
        ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    strcpy(old_ice_p, "FUNCTION=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // process location attribute & add to tag
    ice_add_attribute(old_ice_p, L"i3ce_location", attributeList);
    strcat(old_ice_p, ".");
    
    // process name attribute
    ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ICELevel, FUNCTION, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    return(retval);
} // ice_function_begin }


/* {
** Name: ice_function_end
**
** Description:
**      Push the correct ICE attribute end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_function_end(const int ICELevel) {

    int retval = 0;
    // Terminate an old ICE keyword correctly

    ice_keyword_end(ICELevel, FUNCTION, ice_stack_status);

    return(retval);

} // ice_function_end }
// ice_function }

// ice_if {
/* {
** Name: ice_if_begin
**
** Description:
**      Push the correct ICE if begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_if_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the the element name to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
        ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    // Dont put graves on the tag, the condition tag will do this
    strcpy(old_ice_p, "IF");

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	ICELevel, IF, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
    return(retval);
} // ice_if_start }

// ice_if_end
/* {
** Name: ice_if_end
**
** Description:
**      Push the correct ICE if end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_if_end(const int ICELevel) {

    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;

    // Terminate the tag
    strcpy(old_ice_p, " -->\n");
    
    // Push old_ice onto the stack
    ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	IF, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);

} // ice_if_end }
// ice_if }

// ice_include {
/* {
** Name: ice_include_begin
**
** Description:
**      Push the correct ICE attribute begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_include_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    ice_parameter_count = 0; // So far we've had no parameters

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
        ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    strcpy(old_ice_p, "INCLUDE=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // process location attribute & add to tag
    ice_add_attribute(old_ice_p, L"i3ce_location", attributeList);
    
    // process name attribute
    strcat(old_ice_p, "[");
    ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);
    strcat(old_ice_p, "]");

    // Push old_ice onto the stack, no REPEAT
    ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
        INCLUDE, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
    // Check if we must set the repeat flag (attribute is i3ce_process)
    int attridx = attributeList.getIndex((XMLCh *)L"i3ce_process");
    if (attridx != -1)
    {
	/*
	** if the process variable is 'true' we must set the REPEAT
	** flag for this include
	*/
        if (XMLString::compareNString((XMLCh *)"true",
		attributeList.getValue(attridx),4) == 0)
        {
            // Push old_ice onto the stack with REPEAT set
            ice_set_parent_flags(ICE_REPEAT, igenln);
        }
    }

    return(retval);
} // ice_include_begin }


/* {
** Name: ice_include_end
**
** Description:
**      Push the correct ICE attribute end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_include_end(const int ICELevel) {

    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    // Terminate an old ICE keyword correctly
 
    ice_keyword_end(ICELevel, INCLUDE, ice_stack_status);
   
    return(retval);

} // ice_include_end }
// ice_include }

// ice_html {
/* {
** Name: ice_html_begin
**
** Description:
**      Push the correct ICE attribute begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_html_begin(const int ICELevel, const Attributes&	attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    int level;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    level = (ICELevel == 0) ? 0 : ICELevel -1;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(level, graves_p);

    /*
    ** For the function and include keywords, we must termainate
    ** the previous grave string
    */
    if (OK == ice_test_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln))
    {
	// Add the graves to the tag
	strcpy(old_ice_p, graves);
    }
            
    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, level, 
        HTML, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    // Construct the option
    strcpy(old_ice_p, "\n    HTML=");

    // Put the correct number of grave quotes into the graves array
	SAX2ICETranslate::ice_graves(level, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, level, 
        HTML, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_html_begin }

/* {
** Name: ice_html_end
**
** Description:
**      Push the correct ICE attribute end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_html_end(const int ICELevel) {

    int retval = 0;
    // Terminate an old ICE statement correctly

    // For the function and include keywords, we don't termainate the grave string
    if (OK != ice_test_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln))
    {
	int level;
	char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
	char graves[256];
	char *old_ice_p = old_ice;
	char *graves_p = graves;
		    
	// Put the correct number of grave quotes into the graves array
	level = (ICELevel == 0) ? 0 : ICELevel -1;
	SAX2ICETranslate::ice_graves(level, graves_p);

	// Add the graves to the tag
	strcpy(old_ice_p, graves);
	strcat(old_ice_p, "\n");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    level, HTML, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
    }
    
    return(retval);

} // ice_html_end }
// ice_html }

// ice_option {
/* {
** Name: ice_option_begin
**
** Description:
**      Push the correct begin sequence for an ICE option
**
** Inputs:
**      ICELevel            ICE nesting level
**      ICEKEY              ICE Keyword/Option ID
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_option_begin(const int ICELevel, const enum ICEKEY option, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice; 
    char graves[256];
    char *graves_p = graves;

    // Start with an empty string
    strcpy(old_ice_p, "");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);
	    
    // Construct the tag
    char *attributeString_p;
    int attridx = -1; // -1 indicates not found

    switch (option) {
    // CASE {
    case CASE: 
	strcpy(old_ice_p, "\n    CASE ");
	
	// Add Opening Graves for Value
	strcat(old_ice_p, graves);
	
	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

	// Add Closing Graves for Value
	strcat(old_ice_p, graves);

	// Equals sign for Action
	strcat(old_ice_p, "=");

	// Add Opening Graves for Action
	strcat(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	// The CASE Close must terminate the graves
	ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // CASE }

    // CONDITION begin {
    case CONDITION: 
	
	// Old ICE Syntax: IF ( `lhs` condition `rhs` )
	// Where condition is one of: == OR != OR >= OR <=
	// Add Opening round bracket for Condition, format with spaces
	strcat(old_ice_p, " ( ");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // CONDITION begin }

    // CONDITIONdEFINED begin {
    case CONDITIONdEFINED: 
	
	// Old ICE Syntax: IF ( DEFINED ( variable_name ) )
	// Note: No grave quotes!
	
	// Keyword 'DEFINED' and open round
	strcat(old_ice_p, "DEFINED (");

	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

	// Close the round bracket
	strcat(old_ice_p, ")");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // CONDITIONdEFINED begin }

    // CONDITIONeXPRESSION begin {
    case CONDITIONeXPRESSION: 
	
	// Process Left Hand Side Attribute: i3ce_condlhs
	// Add Opening Graves for Condition lhs
	strcat(old_ice_p, graves);
	// Get Attribute lhs
	ice_add_attribute(old_ice_p, L"i3ce_condlhs", attributeList);
	// Add Closing Graves for Condition lhs
	strcat(old_ice_p, graves);
	
	// Process Conitional Operator Attribute: i3ce_condop
	// prefix with a space for readability 
	strcat(old_ice_p, " ");
	ice_add_attribute(old_ice_p, L"i3ce_condop", attributeList);
	// and append a space for readability 
	strcat(old_ice_p, " ");
	
	// Process Right Hand Side Attribute: i3ce_condrhs 
	// Add Opening Graves for Condition rhs
	strcat(old_ice_p, graves);
	// Get Attribute rhs
	ice_add_attribute(old_ice_p, L"i3ce_condrhs", attributeList);
	// Add Closing Graves for Condition rhs
	strcat(old_ice_p, graves);

	// Push old_ice onto the stack
	/* testing...
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p,
	    STLENGTH( old_ice_p ) + 1, &igenln );
	*/

	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ice_option_level(ICELevel), IF, old_ice_p,
	    STLENGTH( old_ice_p ) + 1, &igenln );

	break; // CONDITIONeXPRESSION begin }

    // EcHILDREN {
    case EcHILDREN: 
        // Extend TAG, marks boundary of a list of Children

	break; // EcHILDREN }
		
    // EcHILD {
    case EcHILD: 
	// Bounds a Child of the Extend TAG

	break; // EcHILD }

    // DEFAULT {
    case DEFAULT: 
		
	strcpy(old_ice_p, "\n    DEFAULT=");
	
	// Add Opening Graves
	strcat(old_ice_p, graves);
	
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // DEFAULT }

    // DESCRIPTION {
    case DESCRIPTION: 
	// Do nothing
	break; // DESCRIPTION }
        
    // EaTTRIBUTES {
    case EaTTRIBUTES: 
        // Dummy TAG, marks boundary of Attribute List
	break; // EaTTRIBUTE }
		
    // EaTTRIBUTE {
    case EaTTRIBUTE: 
	// Bounds an Attribute
        // The Value appears as PCDATA, but must be seperated
	// from the previous by a space
	strcpy(old_ice_p, " ");

        // Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // EaTTRIBUTE }

    // EaTTRIBUTEnAME {
    case EaTTRIBUTEnAME: 
	// Bounds an Attribute Name
        // The name appears as PCDATA

	break; // EaTTRIBUTEnAME }

    // EaTTRIBUTEvALUE {
    case EaTTRIBUTEvALUE: 
		
	// Bounds an Attribute Value
        // The Value appears as PCDATA, but must be seperated
	// from the name by an '='
	strcpy(old_ice_p, "=");

        // Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
			RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // EaTTRIBUTEvALUE }

    // Extend {
    case EXTEND: 
	// Starting an ice extend tag, increase the level.
        // Must not flush till outermost one is ended
        // because we store the tag name on the stack!
        ICEFlush+= 1;
       
        strcpy(old_ice_p, "<");
		
	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_tagName", attributeList);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

        // Push the tag name onto the stack, so that it can be used to
        // construct the end tag later on. Do not allow it to be 
        // extracted for flush (Don't specify the ICE_FLUSH_ENTRY flag).
        strcpy(old_ice_p, "");
        ice_add_attribute(old_ice_p, L"i3ce_tagName", attributeList);
        retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_EXTEND, ICEFlush,
			EXTEND, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
 
	break; // EXTEND }

    // EXT {
    case EXT: 
		
	strcpy(old_ice_p, "\n    EXT=");
		
	// Add Opening Graves
	strcat(old_ice_p, graves);

	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

	// Add Closing Graves
	strcat(old_ice_p, graves);
		
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // EXT }

    // HEADERS {
    case HEADERS: 
	strcpy(old_ice_p, "\n    HEADERS=");

	ice_header_count = 0;
		
	// Add Opening Graves
	strcat(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // HEADERS }

    // HEADER {
    case HEADER: 
	// The first header follows the graves directly 
	// Subsequent headers are seperated by a ', ' from their predecessor
	if(ice_header_count == 0 )
	{
	    // Start with an empty string
	    strcpy(old_ice_p, "");
	}
	else
	{
	    // Continue with a comma space 
	    strcpy(old_ice_p, ", ");
	}
	ice_header_count += 1;
		
	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_column", attributeList);

	strcat(old_ice_p, ", ");
		
	// Process Attribute: target
	ice_add_attribute(old_ice_p, L"i3ce_text", attributeList);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // HEADER }

    // HTML {
    case HTML: 
	strcpy(old_ice_p, "\n    HTML=");
		
	// Add Opening Graves
	strcat(old_ice_p, graves);
		
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // HTML }

    // HYPERLINK {
    case HYPERLINK:

	strcpy(old_ice_p, "\n<a ");
	
	// Push raw HTML onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // HYPERLINK }

    // HYPERLINKaTTRS {
    case HYPERLINKaTTRS:

	// A place holder. Nothing to do here
	// (the work gets done in the close tag.)

	break; // HYPERLINKaTTRS }

    // LINKS {
    case LINKS: 		
	strcpy(old_ice_p, "\n    LINKS=");

	// Zero out link count (used by LINK to decide on seperator, below)
	ice_link_count = 0;
	
	// Add Opening Graves
	strcat(old_ice_p, graves);
	
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // LINKS }

    // LINK {
    case LINK:
	// The first link spec. follows the graves directly 
	// Subsequent links are seperated by a ', ' from their predecessor
	if(ice_link_count == 0 )
	{
		// Start with an empty string
		strcpy(old_ice_p, "");
	}
	else
	{
		// Continue with a comma space 
		strcpy(old_ice_p, ", ");
	}
	
	ice_link_count += 1;
	
	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_column", attributeList);

	strcat(old_ice_p, ", ");

	// Process Attribute: target
	ice_add_attribute(old_ice_p, L"i3ce_target", attributeList);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // LINK }

    // NULLVAR {
    case NULLVAR: 
	
	strcpy(old_ice_p, "\n    NULLVAR=");
	
	// Add Opening Graves
	strcat(old_ice_p, graves);

	// Process Attribute: column
	ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

	// Add Closing Graves
	strcat(old_ice_p, graves);
	
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // NULLVAR }

    // PARAMETER -a parameter {
    case PARAMETER: 
	    
	// Add Opening Graves
	strcpy(old_ice_p, graves);
    
	// Construct the tag part
	// First parameter needs a ? the rest a & seperator
	// Simple flag structure here won't work for nested
	// function calls -which arn't allowed!
	if (ice_parameter_count == 0)
	{
	    // Use a ? on the first (zero level) parameter
	    strcpy(old_ice_p, "?");
	}
	else
	{
	    // Use a & seperator for all other parameters
	    strcpy(old_ice_p, "&");
	}

	// Note how many parameters there are
	ice_parameter_count+= 1;

	// process name attribute
	
	// Process Attribute: name
	ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

	strcat(old_ice_p, "=");
	
	// process Attribute: value
	ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel, 
	    PARAMETER, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // PARAMETER }

    // PARAMETERS -list of parameter {
    case PARAMETERS: 
        // Do nothing
	break; // PARAMETERS }

    // ROWS {
    case ROWS: 
	strcpy(old_ice_p, "\n    ROWS=");
	
	// Add Opening Graves
	strcat(old_ice_p, graves);

	// Process Attributes
	attridx = attributeList.getIndex((XMLCh *)L"i3ce_rowcount");
	if (attridx != -1)
	{
	    // process name attribute
	    attributeString_p =
		XMLString::transcode(attributeList.getValue(attridx));
	    strcat(old_ice_p, attributeString_p);
	    free(attributeString_p);
	}
	else
	{
	    strcpy(old_ice_p, "-1");
	}
	// Add Closing Graves
	strcat(old_ice_p, graves);
	
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	break; // ROWS }

    // TARGET {
    case TARGET: 
	
	strcpy(old_ice_p, "href=");
	
	// Push raw HTML onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // TARGET }

    // XML {
    case XML: 
	strcpy(old_ice_p, "\n    XML=");
		
	// Add Opening Graves
	strcat(old_ice_p, graves);
		
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // XML }

    // XMLPDATA {
    case XMLPDATA: 
	strcpy(old_ice_p, "\n    XMLPDATA=");
		
	// Add Opening Graves
	strcat(old_ice_p, graves);
		
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
	    option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // XMLPDATA }
    // default {
    default:
	// Debug only, if triggered the text "Unknown Option" will
	// cause the old ice parser to issue a syntax error...
	strcpy(old_ice_p, "<!-- Unknown Option -->");
	// Push raw HTML onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY, ICELevel,
		RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // default }
    }

    return(retval);
} // ice_option }

/* {
** Name: ice_option_end
**
** Description:
**      Push the correct end sequence for an ICE option
**
** Inputs:
**      ICELevel    ICE nesting level
**      ICEKEY              ICE Keyword/Option ID
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_option_end(const int ICELevel, const enum ICEKEY option) {

    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;

    // Terminate an old ICE option correctly
    char graves[256];
    char *graves_p = graves;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Construct the tag
    switch (option) {
    // CASE {
    case CASE:
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	// Say we've terminated the graves
	ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);
	break; // CASE }

    // CONDITION end {
    case CONDITION:
	// Close round bracket
	strcpy(old_ice_p, " )");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // CONDITION end }

    // CONDITIONeXPRESSION end {
    case CONDITIONeXPRESSION:
	// Should be nothing to do
	break; // CONDITIONeXPRESSION end }

    // CONDITIONdEFINED {
    case CONDITIONdEFINED:
	// Should be nothing to do
	break; // CONDITIONdEFINED }

    // EcHILDREN {
    case EcHILDREN:
	// Extend TAG, Bounds a list of Children
	break; // EcHILDREN }

    // EcHILD {
    case EcHILD:
        // Extend TAG, Bounds a Child
	break; // EcHILD }

    // DEFAULT {
    case DEFAULT:
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

        // Say we've terminated the graves
        ice_reset_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);
	break; // DEFAULT }

    // DESCRIPTION {
    case DESCRIPTION:		 
	// Nothing to do	
	break; // DESCRIPTION }
        
    // EaTTRIBUTE {
    case EaTTRIBUTE:
        // Extend TAG, Bounds an attribute
	break; // EaTTRIBUTE }

    // EaTTRIBUTES {
    case EaTTRIBUTES:
        // Extend TAG, Bounds a list of attributes
        // Now is the time to terminate the start TAG:
        strcpy(old_ice_p, ">");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // EaTTRIBUTES }

    // EaTTRIBUTEnAME {
    case EaTTRIBUTEnAME:
        // Extend TAG, Bounds an attribute name
        // The '=' is added by the EaTTRIBUTEvALUE option begin code
	break; // EaTTRIBUTEnAME }

    // EaTTRIBUTEvALUE {
    case EaTTRIBUTEvALUE:
        // Extend TAG, Bounds an attribute value
	break; // EaTTRIBUTEvALUE }

    // EXT { 
    case EXT: 
	// Simple case do nothing
	break; // EXT } 

    // EXTEND { 
    case EXTEND: 
	// Terminate the TAG

	strcpy(old_ice_p, "</");
        old_ice_p = old_ice_p + strlen(old_ice_p);
        igen_getstart( pstack, ICE_EXTEND, ICEFlush, old_ice_p);
        strcat(old_ice_p, ">");
 
	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, RAW, old_ice, STLENGTH( old_ice ) + 1, &igenln );

        // Reduce the EXTEND Level
        (ICEFlush== 0) ? ICEFlush = 0 : ICEFlush-= 1;

	break; // EXTEND } 

    // HEADERS { 
    case HEADERS: 
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break;     // HEADERS } 
 
    // HEADER { 
    case HEADER: 
	// Simple case do nothing
	break;     // HEADER } 

    // HTML {
    case HTML:
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // HTML }

    // HYPERLINK {
    case HYPERLINK:
	// Terminate the HTML Anchor tag
	strcpy(old_ice_p, "</a>");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // HYPERLINK }

    // HYPERLINKaTTRS {
    case HYPERLINKaTTRS:
	// Terminate any attributes for the HTML Anchor tag
	strcpy(old_ice_p, ">");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, RAW, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // HYPERLINKaTTRS }

    // LINKS {
    case LINKS:
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // LINKS }

    // LINK {
    case LINK: 
	// Simple case do nothing
	break;	// LINK }

    // NULLVAR {
    case NULLVAR: 
	// Simple case do nothing
	break; // NULLVAR }

    // PARAMETER {
    case PARAMETER: 
		// Simple case do nothing
		break; // PARAMETER }

    // PARAMETERS {
    case PARAMETERS: 

        // PARAMETERS -list of parameter		
	// Add Closing Graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // PARAMETERS }

    // ROWS {
    case ROWS: 
	    // Nothing to do
	break; // ROWS }

    // TARGET {
    case TARGET:		 
	// Nothing to do	
	break; // TARGET }

    // XML {
    case XML:		 
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // XML }

    // XMLPDATA {
    case XMLPDATA :		 
	// Terminate graves
	strcpy(old_ice_p, graves);

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE | ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	break; // XMLPDATA  }

    // default case {
    default:
	// Debug only, if triggered the text "Unknown Option" will break the old ice parser
	strcpy(old_ice_p, "Ending Unknown Option");

	// Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, option, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	break; // default case }
	}


    return(retval);

} // ice_option_end }
// ice_option }

// parameter {
/* {
** Name: ice_parameter_begin
**
** Description:
**      Push the correct ICE parameter begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_parameter_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    int level;               ;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    int retval = 0;

    level = (ICELevel == 0) ? 0 : ICELevel -1;

    // Construct the tag part
    // First parameter needs a ? the rest a & seperator
    // Simple flag structure here won't work for nested function calls
    if (ice_parameter_count == 0)
    {
	// Use a ? on the first (zero level) parameter
	strcpy(old_ice_p, "?");
    }
    else
    {
	// Use a & seperator for all other parameters
	strcpy(old_ice_p, "&");
    }

    // Note how many parameters there are: only close on the last one
    ice_parameter_count+= 1;
    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    // process name attribute
    ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

    // process value attribute
    strcat(old_ice_p, "=");
    ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY, level, 
	PARAMETER, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_parameter_begin }

/* {
** Name: ice_parameter_end
**
** Description:
**      Push the correct ICE parameter end sequence
**      Does nothing in this case
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_parameter_end(const int ICELevel) {

    // Terminate an old ICE statement correctly
    int retval = 0;
    // Nothing to do

    return(retval);

} // ice_parameter_end }
// parameter }

// ice_parent {
/* {
** Name: ice_parent_begin
**
** Description:
**      Push the correct ICE parent begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      ICEKeyword          ID of ICE Keyword
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_parent_begin(const int ICELevel, const enum ICEKEY ICEKeyword, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;
    char graves[256];
    char *graves_p = graves;


    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    switch(ICEKeyword)
    {
		// SWITCH {
	case SWITCH:
	    strcpy(old_ice_p, "SWITCH=");
		
	    // Add Opening Graves
	    strcat(old_ice_p, graves);

	    // Process Attribute: column
	    ice_add_attribute(old_ice_p, L"i3ce_value", attributeList);

	    // Add Closing Graves
	    strcat(old_ice_p, graves);

	    // Push old_ice onto the stack
	    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
		ICELevel, SWITCH, old_ice, STLENGTH( old_ice ) + 1, &igenln );

	    break; // SWITCH }

	case SYSTEM:
	    /* System function is like user function but there is no location */
	    strcpy(old_ice_p, "FUNCTION=");
		
	    // Add Opening Graves
	    strcat(old_ice_p, graves);

	    // Process Attribute: name
	    ice_add_attribute(old_ice_p, L"i3ce_name", attributeList);

	    strcat(old_ice_p, "?action=");
	    // Process Attribute: action
	    ice_add_attribute(old_ice_p, L"i3ce_action", attributeList);
	    // this is the first parameter: register the fact
	    ice_parameter_count = 1;

	    // Push old_ice onto the stack
	    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
		ICELevel, SWITCH, old_ice, STLENGTH( old_ice ) + 1, &igenln );

	    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

	    break; // SYSTEM }

	default:
	    // Create the error message
	    XMLCh error_w[MAX_OLD_ICE_STATEMENT_LENGTH];
	    XMLCh *error_wp = error_w;

	    XMLString::copyString(error_wp,
		(XMLCh *)"\n<!-- ICE Unknown Tag Begin -->");

	    // Push the tag onto the stack in local form
	    char *html_out_chp ;

	    html_out_chp = XMLString::transcode(error_w);

	    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
		ICELevel, UNKNOWN, html_out_chp, strlen(html_out_chp) + 1,
		&igenln);
	    free(html_out_chp);
	    break;
    }
    return(retval);

} // ice_parent_begin }

/* {
** Name: ice_parent_end
**
** Description:
**      Push the correct ICE parent end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**      ICEKeyword  ID of ICE Keyword
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_parent_end(const int ICELevel, const enum ICEKEY ICEKeyword) {

    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;
    char graves[256];
    char *graves_p = graves;


    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    switch(ICEKeyword)
    {
	// SWITCH {
	case SWITCH:
	    // Terminate the tag
	    strcpy(old_ice_p, " -->\n");
		
	    // Push old_ice onto the stack
	    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
		ICELevel, ICECLOSE, old_ice_p, STLENGTH( old_ice_p ) + 1,
		&igenln );

	break; // SWITCH }

	// SYSTEM {
	case SYSTEM:
	    // Terminate the tag
	    ice_keyword_end(ICELevel, FUNCTION, ice_stack_status);

	break; // SYSTEM }

	// default case {
	default:
	    // Create the error message
	    strcpy(old_ice_p, "\n<!-- ICE Unknown Tag End -->\n");

	    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
		ICELevel, UNKNOWN, old_ice, strlen(old_ice) + 1, &igenln);
	    break; // default case }
	}

    return(retval);
} // ice_parent_end }
// ice_parent }

// query {
/* {
** Name: ice_query_begin
**
** Description:
**      Push the correct ICE query begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_query_begin(const int ICELevel, const   Attributes& attributeList) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement

    int retval = 0;
    int attr_idx;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char *old_ice_p = old_ice;
    char graves[256];
    char *graves_p = graves;
    char *attributeString_p;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);
    
    // clear out the old_ice array:
    strcpy(old_ice_p, "");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Construct the rest of the tag

    // 1. process database attributes
    attr_idx = attributeList.getIndex((XMLCh *)L"i3ce_database"); 
    if (attr_idx != -1) {
	strcpy(old_ice_p, "\n    DATABASE=");
	strcat(old_ice_p, graves);

	// 1a. process any vnode attribute
	// do not use ice_add_attribute, because we conditionally
	// add the '::' seperator iff the vnode is specified.
	unsigned int attr_idx = 0;
	attr_idx = attributeList.getIndex((XMLCh *)L"i3ce_vnode");
	if(attr_idx != -1) {
	    attributeString_p =
		XMLString::transcode(attributeList.getValue(attr_idx));
	    strcat(old_ice_p, attributeString_p);
	    strcat(old_ice_p, "::");
	    free(attributeString_p);
	}
	// 1b. process the database name attribute
	ice_add_attribute(old_ice_p, L"i3ce_database", attributeList);

	// 1c. terminate database specification
	strcat(old_ice_p, graves);
    }


    // 2. process any user attribute
    attr_idx = attributeList.getIndex((XMLCh *)L"i3ce_user");
    if (attr_idx != -1) {
	attributeString_p =
	    XMLString::transcode(attributeList.getValue(attr_idx));
	strcat(old_ice_p, "\n    USERID=");
	strcat(old_ice_p, graves);
	strcat(old_ice_p, attributeString_p);
	strcat(old_ice_p, graves);
	free(attributeString_p);
    }

    // 3. process any password attribute
    attr_idx = attributeList.getIndex((XMLCh *)L"i3ce_password");
    if (attr_idx != -1) {
	attributeString_p =
	    XMLString::transcode(attributeList.getValue(attr_idx));
	strcat(old_ice_p, "\n    PASSWORD=");
	strcat(old_ice_p, graves);
	strcat(old_ice_p, attributeString_p);
	strcat(old_ice_p, graves);
	free(attributeString_p);
    }

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, PARAMETER, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_query_begin }

/* {
** Name: ice_query_end
**
** Description:
**      Push the correct ICE query end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_query_end(const int ICELevel) {

	int retval = 0;
	// Terminate an old ICE statement correctly

 	char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
	char *old_ice_p = old_ice;

    // Terminate the tag
	strcpy(old_ice_p, " -->\n");
	
    // Push old_ice onto the stack
	retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, QUERY, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

	return(retval);

} // ice_query_end }

// query }

// ice_relation_display {
/* {
** Name: ice_relation_display_begin
**
** Description:
**      Push the correct ICE relation_display begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_relation_display_begin(const int ICELevel, const   Attributes& attributeList) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE option
    int retval = 0;
    int level;
    XMLCh old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *old_ice_p = old_ice;
    XMLCh option[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *option_p = option;
    XMLCh wgraves[256];
    XMLCh *wgraves_p = wgraves;

    // The option level is the same as the parent keyword level
    level = (ICELevel == 0) ? 0 : ICELevel -1;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_wgraves(level, wgraves_p);

    // The SQL Option has the following optional attributes:
    // i3ce_cursor
    // i3ce_transaction

    // Process Attributes
    // Ensure we start with an empty string
    XMLString::copyString(old_ice_p, (XMLCh *)"\n");

    unsigned int len = attributeList.getLength();
    for (unsigned int index = 0; index < len; index++)
    {
	if ( fExpandNS )
	{
	    if (XMLString::compareIString(attributeList.getURI(index),
		XMLUni::fgEmptyString) != 0)
	    {
		XMLString::catString(old_ice_p, stColon ) ;
	    }
	    XMLString::copyString(option_p,
		attributeList.getLocalName(index) ) ;
	}
	else
	{
	    XMLString::copyString(option_p,
		attributeList.getQName(index) ) ;
	}

	// Which option?
	if (0 == XMLString::compareIString((XMLCh *)L"i3ce_typename", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"    TYPE");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_transaction", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"TRANSACTION");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_cursor", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"CURSOR");
	} else {
	    // error! Unknown SQL Option!
	    XMLString::catString(old_ice_p, option_p);
	}

	XMLString::catString(old_ice_p, stEqual);
	XMLString::catString(old_ice_p, wgraves);

	XMLString::copyString(option_p, attributeList.getValue(index) );

	// Which format?
	// Can be one of: TABLE|SELECTOR|PLAIN|UNFORMATTED|XML|XMLPDATA
	if (0 == XMLString::compareIString((XMLCh *)L"i3ce_table", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"TABLE");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_selector", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"SELECTOR");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_plain", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"PLAIN");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_unformatted", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"UNFORMATTED");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_xmlpdata", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"XMLPDATA");
	}
	else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_xml", option_p))
	{
	    XMLString::catString(old_ice_p, (XMLCh *)L"XML");
	}
	else
	{
	    // error! Unknown display format!
	    XMLString::catString(old_ice_p, option_p);
	}

	XMLString::catString(old_ice_p, wgraves);
	XMLString::catString(old_ice_p, (XMLCh *)L"\n");
    }

    // XMLString::catString(old_ice_p, stCloseAngle);

    // Push the tag onto the stack in local form
    char *html_out_chp ;

    html_out_chp = XMLString::transcode(old_ice_p);

    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, RAW, html_out_chp, strlen(html_out_chp) + 1, &igenln);
    free(html_out_chp);

    return(retval);
} // ice_relation_display_begin }

/* {
** Name: ice_relation_display_end
**
** Description:
**      Push the correct ICE relation_display end sequence
**      Does nothing
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_relation_display_end(const int ICELevel) {

    // Terminate an old ICE statement correctly
    int retval = OK;

    // Nothing to do

    return(retval);

} // ice_relation_display_end }
// relation_display }

// ice_rollback {
/* {
** Name: ice_rollback_begin
**
** Description:
**      Push the correct ICE rollback begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_rollback_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;
    char *attributeString_p;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, ICEOPEN, (char *)"\n<!-- #ICE ",12, &igenln);

    // Construct the tag
    strcpy(old_ice_p, "ROLLBACK=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // process scope attribute & add to tag
    attributeString_p =
	XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_transaction"));
    strcat(old_ice_p, attributeString_p);
    free(attributeString_p);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, ROLLBACK, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    return(retval);
} // rollback_begin  }

/* {
** Name: ice_rollback_end
**
** Description:
**      Push the correct ICE rollback end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_rollback_end(const int ICELevel) {

    int retval = 0;

    // Terminate an old ICE keyword correctly
    ice_keyword_graves_end(ICELevel, ROLLBACK, ice_stack_status);

    return(retval);

} // ice_rollback_end }
// ice_rollback }

// sql {
/* {
** Name: ice_sql_begin
**
** Description:
**      Push the correct ICE sql begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_sql_begin(const int ICELevel, const   Attributes& attributeList) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE option
    int retval = 0;
    int level;
    XMLCh old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *old_ice_p = old_ice;
    XMLCh option[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *option_p = option;
    XMLCh wgraves[256];
    XMLCh*wgraves_p = wgraves;

    // The option level is the same as the parent keyword level
    level = (ICELevel == 0) ? 0 : ICELevel -1;

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_wgraves(level, wgraves_p);

    // The SQL Option has the following optional attributes:
    // i3ce_cursor
    // i3ce_transaction

    // Process Attributes
    // Ensure we start with an empty string
    XMLString::copyString(old_ice_p, (XMLCh *)"\0");

    unsigned int len = attributeList.getLength();
    if(len > 0 ) 
    {
	for (unsigned int index = 0; index < len; index++)
	{
	    if ( fExpandNS )
	    {
		if (XMLString::compareIString(attributeList.getURI(index),
		    XMLUni::fgEmptyString) != 0)
		{
			XMLString::catString(old_ice_p, stColon ) ;
		}

		XMLString::copyString(option_p,
		    attributeList.getLocalName(index) ) ;
	    }
	    else
	    {
		XMLString::copyString(option_p, attributeList.getQName(index) ) ;
	    }
	    // Which option?
	    if (0 == XMLString::compareIString((XMLCh *)L"i3ce_transaction", option_p))
	    {
		    XMLString::catString(old_ice_p, (XMLCh *)L"\n    TRANSACTION");
	    }
	    else if (0 == XMLString::compareIString((XMLCh *)L"i3ce_cursor", option_p))
	    {
		    XMLString::catString(old_ice_p, (XMLCh *)L"\n    CURSOR");
	    } 
	    else 
	    {
		// error! Unknown SQL Option!
		XMLString::catString(old_ice_p, option_p);
	    }
		XMLString::catString(old_ice_p, stEqual);
		XMLString::catString(old_ice_p, wgraves);
		XMLString::catString(old_ice_p, attributeList.getValue(index));
		XMLString::catString(old_ice_p, wgraves);
		XMLString::catString(old_ice_p, (XMLCh *)"\n");
	}

	// Push the tag onto the stack in local form
	char *html_out_chp ;

	html_out_chp = XMLString::transcode(old_ice_p);

	retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	    ICELevel, SQL, html_out_chp, strlen(html_out_chp) + 1, &igenln);
	free(html_out_chp);

    } /* if len > 0 */


    return(retval);
} // ice_sql_begin }

/* {
** Name: ice_sql_end
**
** Description:
**      Push the correct ICE sql end sequence
**      Does nothing
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_sql_end(const int ICELevel) {
    // Terminate an old ICE statement correctly
    int retval = 0;
    // Nothing to do
    return(retval);

} // ice_sql_end }
// sql }

// statement {
/* {
** Name: ice_statement_begin
**
** Description:
**      Push the correct ICE statement begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_statement_begin(const int ICELevel, const   Attributes& attributeList) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE option

    int retval = 0;
    int level;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    level = (ICELevel == 0) ? 0 : ICELevel -1;


    // Push the start of the SQL (mandatory) option
    strcpy(old_ice_p, "\nSQL=");
        
    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // There are no attributes, this tag pair encloses plain text...

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	level, STATEMENT, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );
	
    return(retval);
} // ice_statement_begin }

/* {
** Name: ice_statement_end
**
** Description:
**      Push the correct ICE statement end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_statement_end(const int ICELevel) {
    // Terminate an old ICE statement correctly
    int retval = OK;
    int level;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    level = (ICELevel == 0) ? 0 : ICELevel -1;

    for(int i = 0; i < 256; i++) graves[i] = '\0';

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(level, graves_p);
    strcpy(old_ice_p, graves);
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY, level, 
        STATEMENT, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);
} // ice_statement_end }
// statement }

// ice_then {
/* {
** Name: ice_then_begin
**
** Description:
**      Push the correct ICE then begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_then_begin(const int ICELevel, const   Attributes& attributeList) {

    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = OK;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;

    // Construct the tag
    strcpy(old_ice_p, "\nTHEN=");

    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);
    
    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ice_option_level(ICELevel), IF, old_ice_p,
	STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);

} // ice_then_begin }

/* {
** Name: ice_then_end
**
** Description:
**      Push the correct ICE then end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_then_end(const int ICELevel) {

    // Terminate an old ICE keyword correctly
    int retval = 0;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;


    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ice_option_level(ICELevel), graves_p);

    // Add the graves to the tag
    strcpy(old_ice_p, graves);
 	
    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, THEN, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    return(retval);

} // ice_then_end }
// ice_then }

// ice_var {
/* {
** Name: ice_var_begin
**
** Description:
**      Push the correct ICE var begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_var_begin(const int ICELevel, const   Attributes& attributeList) {

     // Combine the grave quotes with the attributes to form
     // up a correct old ICE statement
    int retval = OK;
    char old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    char graves[256];
    char *old_ice_p = old_ice;
    char *graves_p = graves;
    char *attributeString_p;

    // Push the ICE start sequence
    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, ICEOPEN, (char *)"<!-- #ICE ",11, &igenln);

    // Construct the rest of the tag
    strcpy(old_ice_p, "VAR=");
        
    // Put the correct number of grave quotes into the graves array
    SAX2ICETranslate::ice_graves(ICELevel, graves_p);

    // Add the graves to the tag
    strcat(old_ice_p, graves);

    // Future: process scope attribute
//	attributeString_p = XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_scope"));
//	strcat(old_ice_p, attributeString_p);
//	free(attributeString_p);
//		
//	strcat(old_ice_p, ".");
//		    
    // process name attribute
    attributeString_p =
	XMLString::transcode(attributeList.getValue((XMLCh *)L"i3ce_name"));
    // Must introduce the name with a colon ':'
    strcat(old_ice_p, ":");
    strcat(old_ice_p, attributeString_p);
    free(attributeString_p);

    // Push old_ice onto the stack
    retval = ice_push_chk( pstack,  ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, VAR, old_ice_p, STLENGTH( old_ice_p ) + 1, &igenln );

    ice_set_parent_flags(ICE_FLAG_GRAVES_OPENED, igenln);

    return(retval);
} // ice_var_begin }

/* {
** Name: ice_var_end
**
** Description:
**      Push the correct ICE var end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_var_end(const int ICELevel) {

     // Any extra processing needed to terminate an old ice statement
    int retval = 0;

    // Terminate an old ICE keyword correctly
    ice_keyword_graves_end(ICELevel, VAR, ice_stack_status);

    return(retval);
} // ice_var_end }
// ice_var }

// ice_unknown {
/* {
** Name: ice_unknown_begin
**
** Description:
**      Push the correct ICE unknown begin sequence
**
** Inputs:
**      ICELevel            ICE nesting level
**      attributeList       SAX Structure containing XML attributes
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_unknown_begin(const int ICELevel, const XMLCh *qname) {
    // Combine the grave quotes with the attributes to form
    // up a correct old ICE statement
    int retval = 0;
    char graves[256];
    char *graves_p = graves;
    XMLCh old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *old_ice_p = old_ice;

    // Create the error message
    XMLString::copyString(old_ice_p,
	(XMLCh *)L"\n<!-- ICE Unknown Tag Begin (tag name: ");
    XMLString::catString(old_ice_p, qname);
    XMLString::catString(old_ice_p, (XMLCh *)L") -->\n");

    // Push the tag onto the stack in local form
    char *html_out_chp ;

    html_out_chp = XMLString::transcode(old_ice);

    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, UNKNOWN, html_out_chp, strlen(html_out_chp) + 1, &igenln);
    free(html_out_chp);

    return(retval);

} // unknown_begin }

/* {
** Name: ice_unknown_end
**
** Description:
**      Push the correct ICE unknown end sequence
**
** Inputs:
**      ICELevel    ICE nesting level
**
** Outputs:
**      None
**
** Return:
**      OK                      Command completed successfully.
**      E_STK_INVALID_STACK     An invalid stack pointer supplied.
**      E_IGEN_INVALID_PARAM    An invalid return pointer supplied.
**
** History:
**      27-Mar-2001 (peeje01)
**          Created.
*/
int SAX2ICETranslate::ice_unknown_end(const int ICELevel, const XMLCh *qname) {

     // Any extra processing needed to terminate an old ice statement
    int retval = 0;
    XMLCh old_ice[MAX_OLD_ICE_STATEMENT_LENGTH];
    XMLCh *old_ice_p = old_ice;

    // Create the error message
    XMLString::copyString(old_ice_p,
	(XMLCh *)L"\n<!-- ICE Unknown Tag End (tag name: ");
    XMLString::catString(old_ice_p, qname);
    XMLString::catString(old_ice_p, (XMLCh *)L") -->\n");

    // Push the tag onto the stack in local form
    char *html_out_chp ;

    html_out_chp = XMLString::transcode(old_ice);

    retval = ice_push_chk(pstack, ICE_ENTRY_INUSE |  ICE_FLUSH_ENTRY,
	ICELevel, UNKNOWN, html_out_chp, strlen(html_out_chp) + 1, &igenln);
    free(html_out_chp);

    return(retval);
} // unknown_end }
// ice_unknown }
