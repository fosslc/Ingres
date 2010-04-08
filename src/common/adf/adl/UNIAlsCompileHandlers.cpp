/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: UNIAlsCompileHandlers.cpp - Handle functions for unialscompile.
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
**	30-jan-2004 (somsa01)
**	    Updated so that this can build with .NET 2003, as the iostream
**	    headers and libraries have changed for .NET 2003.
**	07-feb-2004 (gupsh01)
**	    Fixed unialias compile, which was failing due to filesize 
**	    not being in exact multiple of COL_BLOCK.
**	15-mar-2004 (gupsh01)
**	    Modified to support Xerces 2.3.
**      29-mar-2004 (gupsh01)
**          Put back building with changed iostream with .Net accidently
**          overwritten by a previous change.
**      18-aug-2004 (devjo01)
**          Undefine min & max macros before including the *hpp files
**          in order to avoid compile error in some build environments..
**      15-Sep-2004 (bonro01)
**          endif can only be followed by a comment on HP.
**	03-Jul-2007 (drivi01)
**	    Enclose headers being included in 'extern "C"' to
**	    avoid compile problems.
**	21-Sep-2009 (hanje04)
**	   Use new include method for stream headers on OSX (DARWIN) to
**	   quiet compiler warnings.
**	   Also add version specific definitions of parser funtions to
**	   allow building against Xerces 2.x and 3.x
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

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(NT_GENERIC) || defined(LINUX) || defined(DARWIN)
#include <iostream>
using namespace std;
#else
#include <iostream.h>
#endif


extern "C"
{
# include <compat.h>
# include <cv.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <st.h>
# include <gl.h>
# include <cm.h>
# include <iicommon.h>
# include <fe.h>
# include <aduucoerce.h>

# ifdef min
# undef min
# endif /*min*/
# ifdef max
# undef max
# endif /*max*/

} //extern "C"

# include <xercesc/util/XMLUni.hpp>
# include <xercesc/util/XMLUniDefs.hpp>
# include <xercesc/sax/AttributeList.hpp>
# include "UNIAlsCompile.hpp"

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
static const XMLCh gStartComment[] = { chOpenAngle, chBang, chDash, chDash, 
									   chNull };
static const XMLCh gEndComment[] = { chDash, chDash, chCloseAngle, chNull };
static const XMLCh stOpenAngle[2] = {chOpenAngle, L'\0'};
static const XMLCh stCloseAngle[2] = {chCloseAngle, L'\0'};
static const XMLCh stSpace[2] = {chSpace, L'\0'};
static const XMLCh stEqual[2] = {chEqual, L'\0'};
static const XMLCh stDoubleQuote[2] = {chDoubleQuote, L'\0'};
static const XMLCh stColon[2] = {chColon, L'\0'};

STATUS status = OK;

// ---------------------------------------------------------------------------
//  UNIAlsCompileHandlers: Constructors and Destructor
// ---------------------------------------------------------------------------
UNIAlsCompileHandlers::UNIAlsCompileHandlers( const   char* const encodingName
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
	mapindex   = 0;
	aliasindex = 0;
	mapcount   = 0;
	aliascount = 0;
}

UNIAlsCompileHandlers::~UNIAlsCompileHandlers()
{

}

// ---------------------------------------------------------------------------
//  UNIAlsCompileHandlers: Overrides of the output formatter target interface
// ---------------------------------------------------------------------------
void UNIAlsCompileHandlers::writeChars(const XMLByte* const toWrite)
{
	cout<<"in writeChars"<<endl;
}


void UNIAlsCompileHandlers::writeChars(const XMLByte* const toWrite,
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
//  UNIAlsCompileHandlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void UNIAlsCompileHandlers::error(const SAXParseException& e)
{
    cerr << "\nError at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void UNIAlsCompileHandlers::fatalError(const SAXParseException& e)
{
    cerr << "\nFatal Error at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void UNIAlsCompileHandlers::warning(const SAXParseException& e)
{
    cerr << "\nWarning at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

// ---------------------------------------------------------------------------
//  UNIAlsCompileHandlers: Overrides of the SAX DTDHandler interface
// ---------------------------------------------------------------------------
void UNIAlsCompileHandlers::unparsedEntityDecl(const     XMLCh* const name
                                          , const   XMLCh* const publicId
                                          , const   XMLCh* const systemId
                                          , const   XMLCh* const notationName)
{
    // Not used at this time
}

void UNIAlsCompileHandlers::notationDecl(const   XMLCh* const name
                                    , const XMLCh* const publicId
                                    , const XMLCh* const systemId)
{
    // Not used at this time
}

// ---------------------------------------------------------------------------
//  UNIAlsCompileHandlers: Overrides of the SAX DocumentHandler interface
// ---------------------------------------------------------------------------
void UNIAlsCompileHandlers::characters(const     XMLCh* const    chars,
                                   INGXMLSize_t length)
{ 
	fFormatter.formatBuf(chars, length, XMLFormatter::CharEscapes);
}

void UNIAlsCompileHandlers::endDocument() { }


void UNIAlsCompileHandlers::endElement(const XMLCh* const name)
{
    if (!strncmp("mapping", StrX(name).localForm(), 7)) 
	mapindex++;
    else if (!strncmp("alias", StrX(name).localForm(), 5))
	aliasindex++;
    else if (!strncmp("characterMappingAliases", StrX(name).localForm(),23))
    {
      if (writedata() != OK) 
        cout <<"ALIAS COMPILE: Cannot write to the alias file "<<endl;
    }
}

void UNIAlsCompileHandlers::ignorableWhitespace( const   XMLCh* const chars,
                                   INGXMLSize_t length)
{ }


void UNIAlsCompileHandlers::processingInstruction(const  XMLCh* const target
                                            , const XMLCh* const data)
{
    fFormatter << XMLFormatter::NoEscapes << gStartPI  << target;
    if (data)
        fFormatter << chSpace << data;
    fFormatter << XMLFormatter::NoEscapes << gEndPI;
}


void UNIAlsCompileHandlers::startDocument() { }


void UNIAlsCompileHandlers::startElement(const   XMLCh* const    name,
                                AttributeList&	attributes)
{
    unsigned int index = 0; 
    char *const target = 0; 
    unsigned int len;

    if (!strncmp("mapping", StrX(name).localForm(), 7))
    {
      mapcount++;

      len = attributes.getLength();
      for (index=0; index < len; index++)
      {
        /* get id */
        if (!strncmp("id", (StrX(attributes.getName(index))).localForm(), 2))
        {
	  XMLString::copyString(alMappingArray[mapindex].mapping_id, 
	       (StrX(attributes.getValue(index))).localForm());
        }
      }
    }
    else if (!strncmp("display", StrX(name).localForm(), 7))
    {
      /* get display name */
      len = attributes.getLength();
      for (index=0; index < len; index++)
      {
        /* get display name */
        if (!strncmp("name", (StrX(attributes.getName(index))).localForm(), 4))
        {
          XMLString::copyString(alMappingArray[mapindex].display_name,
               (StrX(attributes.getValue(index))).localForm());
        }
        /* get xml:lang */
        else if (!strncmp("xml:lang", 
		  (StrX(attributes.getName(index))).localForm(), 8))
        {
          XMLString::copyString(alMappingArray[mapindex].display_xmllang,
               (StrX(attributes.getValue(index))).localForm());
        }
      }
    }
    else if (!strncmp("alias", StrX(name).localForm(), 5))
    {
      aliascount++;
      /* get alias name */
      len = attributes.getLength();
      for (index=0; index < len; index++)
      {
        /* get display */
        if (!strncmp("name", 
		  (StrX(attributes.getName(index))).localForm(), 4))
        {
          XMLString::copyString(alAliasArray[aliasindex].aliasName,
                (StrX(attributes.getValue(index))).localForm());
        }
        /* get preferredBy information */
        else if (!strncmp("preferredBy", 
		  (StrX(attributes.getName(index))).localForm(), 11))
        {
          XMLString::copyString(alAliasArray[aliasindex].aliasPrefBy,
                (StrX(attributes.getValue(index))).localForm());
        }
        alAliasArray[aliasindex].aliasMapId = mapindex;
	  /* normalize the data and place in aliasNameNorm */
      }
      normalize(alAliasArray[aliasindex].aliasName, 
	  strlen(alAliasArray[aliasindex].aliasName), 
	  alAliasArray[aliasindex].aliasNameNorm);
    } 
}

void  UNIAlsCompileHandlers::normalize(char *instring, i4 inlength, char *outstring)
{
    char *iptr = instring;
    char *endinput = instring + inlength;
    char *optr = outstring;
    char nopreceding = '0';

    while (iptr < endinput)
    {
      if (CMdigit(iptr) || CMalpha(iptr))
      {
    	if (CMalpha(iptr))
    	  nopreceding = *iptr;
    	else 
    	  nopreceding = '0';

    	CMtolower(iptr,iptr);

    	/* From left to right, delete each 0 that is not preceded by a digit. 
    	** For example, the following names should match: "UTF-8", "utf8", 
    	** "u.t.f-008", but not "utf-80" or "ut8".
    	*/
    	if (nopreceding != '0'&& *iptr == '0')
    	{
    	  *iptr++;
    	  continue; 
    	}
	*optr++ = *iptr++;
      }
      else 
        iptr++;
    }
    *optr = '\0'; 
}

STATUS UNIAlsCompileHandlers::writedata()
{
    /* create a directory $II_SYSTEM/ingres/files/charmaps */
    CL_ERR_DESC 	syserr;
    char		*alias_buffer; 
    char		*tptr;
    SIZE_TYPE   	tlen = 0;
    SIZE_TYPE	*sizep;
    SIZE_TYPE	*datasize;

    alias_buffer = (char *)MEreqmem(0, MAX_ALSTAB_SIZE, TRUE, &status);
   /* 
    if (CMopen_col("aliasmaptbl", &syserr, CM_UCHARMAPS_LOC) != OK)
    {
    cout <<"ALIAS COMPILE ERROR: Cannot open alias file for writing\n"<<endl;
    	return FAIL;
    }
    */

    /* 
    ** Write the information to the buffer.
    */
    tptr = alias_buffer;
    /* data size is the first entry of the map file */
    datasize = (SIZE_TYPE *) tptr; 
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* write the count of the alMappingArray nodes */
    sizep = (SIZE_TYPE *) tptr; 
    *sizep = mapcount * sizeof(ADU_ALIAS_MAPPING);
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* write the ADU_ALIAS_MAPPING array */
    MEcopy(alMappingArray, *sizep, tptr);
    tptr += *sizep; 
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
    
    /* write the count of the alAliasArray nodes */
    sizep = (SIZE_TYPE *) tptr; 
    *sizep = aliascount * sizeof(ADU_ALIAS_DATA);
    tptr += sizeof(SIZE_TYPE);
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* write the ADU_ALIAS_DATA array */
    MEcopy(alAliasArray, *sizep, tptr);
    tptr += *sizep; 
    tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

    /* update the length of the buffer filled */
    tlen = ((tptr - alias_buffer) + COL_BLOCK) & ~(COL_BLOCK-1);
    *datasize = tlen;

    /* write the buffer to the "aliasmaptbl" file */
    if( CMdump_col("aliasmaptbl", (PTR)alias_buffer, tlen, 
    			&syserr, CM_UCHARMAPS_LOC) != OK)
    {
      cout <<"ALIAS COMPILE ERROR: Cannot write to the alias file \n"<<endl;
      return FAIL;
    }
    return OK;
}
