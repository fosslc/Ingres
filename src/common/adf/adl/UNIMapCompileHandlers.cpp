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
**      14-Jan-2004 (gupsh01)
**          Created.
**	23-Jan-2004 (gupsh01)
**	    Fixed up for correctly compiling the mapping file.
**	30-jan-2004 (somsa01)
**	    Updated so that this can build with .NET 2003, as the iostream
**	    headers and libraries have changed for .NET 2003.
**	15-mar-2004 (gupsh01)
**	    Modified to support Xerces 2.3.
**      29-mar-2004 (gupsh01)
**          Put back building with changed iostream with .Net accidently
**          overwritten by a previous change.
**      18-aug-2004 (devjo01)
**          Undefine min & max macros before including the *hpp files
**          in order to avoid compile error in some build environments..
**	08-sep-2004 (gupsh01)
**	    Reset the map_header pointers after the buffer for map_tbl 
**	    which was filled up is increased.
**      15-Sep-2004 (bonro01)
**          endif can only be followed by a comment on HP
**      24-Sep-2008 (macde01)
**          Bug 120943 - increase size of charbyte by 3 bytes, as the value
**          from the xml file may be more than 8 bytes.
**	24-Apr-2009 (bonro01)
**	    Fix alignment of the validity structures written to the
**	    64bit ucharmap files for 64bit platforms.
**	    Clear map_validity structures so garbage characters are not
**	    written to the ucharmap files.
**	01-May-2009 (bonro01)
**	    Previous change fixed validity alignment but caused start of
**	    assignments structures to be unaligned for 64bit platforms.
**	11-May-2009 (bonro01)
**	    Fix problem handling four byte values for asssignment byte codes
**	    which occurs in ca-euctw-chteeuc-2005.xml
**	    Add error messages for invalid values.
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

#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include "UNIMapCompile.hpp"


//#include <erwu.h>

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
//  UNIMapCompileHandlers: Constructors and Destructor
// ---------------------------------------------------------------------------
UNIMapCompileHandlers::UNIMapCompileHandlers( const   char* const encodingName
                                    , const XMLFormatter::UnRepFlags unRepFlags
			 	    , const bool expandNamespaces
				    , char *outFile) : 

	fFormatter
    (
        encodingName, 0
        , this
        , XMLFormatter::NoEscapes
        , unRepFlags
    ),
	fExpandNS ( expandNamespaces )
{
        validity_cnt = 0;
        assignment_cnt = 0;
        map_tbl_size = 0;
	buffer_size = 0;

	charset = outFile;

        subchar_ptr = NULL;
        subchar1_ptr = NULL;
        tbl_size_ptr = NULL;
	validity_size_ptr = NULL;
	assignment_size_ptr = NULL;
	map_tbl = NULL;
	tptr = NULL;
	align_validities = FALSE;
}

// ---------------------------------------------------------------------------
//  UNIMapCompileHandlers: Overrides of the output formatter target interface
// ---------------------------------------------------------------------------
void UNIMapCompileHandlers::writeChars(const XMLByte* const toWrite)
{
	cout<<"in writeChars"<<endl;
}

void UNIMapCompileHandlers::writeChars(const XMLByte* const toWrite,
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
//  UNIMapCompileHandlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void UNIMapCompileHandlers::error(const SAXParseException& e)
{
    cerr << "\nError at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void UNIMapCompileHandlers::fatalError(const SAXParseException& e)
{
    cerr << "\nFatal Error at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

void UNIMapCompileHandlers::warning(const SAXParseException& e)
{
    cerr << "\nWarning at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
         << "\n  Message: " << StrX(e.getMessage()) << endl;
}

// ---------------------------------------------------------------------------
//  UNIMapCompileHandlers: Overrides of the SAX DTDHandler interface
// ---------------------------------------------------------------------------
void UNIMapCompileHandlers::unparsedEntityDecl(const     XMLCh* const name
                                          , const   XMLCh* const publicId
                                          , const   XMLCh* const systemId
                                          , const   XMLCh* const notationName)
{
    // Not used at this time
}

void UNIMapCompileHandlers::notationDecl(const   XMLCh* const name
                                    , const XMLCh* const publicId
                                    , const XMLCh* const systemId)
{
    // Not used at this time
}

// ---------------------------------------------------------------------------
//  UNIMapCompileHandlers: Overrides of the SAX DocumentHandler interface
// ---------------------------------------------------------------------------
void UNIMapCompileHandlers::characters(const     XMLCh* const    chars,
                                   INGXMLSize_t length)
{ 
	fFormatter.formatBuf(chars, length, XMLFormatter::CharEscapes);
}

void UNIMapCompileHandlers::endDocument() 
{ 
	if (map_tbl)
	  MEfree(map_tbl);
	map_tbl = NULL;
}

void UNIMapCompileHandlers::endElement(const XMLCh* const name)
{
  if (!strncmp("characterMapping", StrX(name).localForm(),23))
  {
    map_tbl_size = tptr - map_tbl;
    //    *tbl_size_ptr = map_tbl_size;
    if (writedata() != OK) 
      cout <<"ALIAS COMPILE: Cannot write to the alias file "<<endl;
  }
}

void UNIMapCompileHandlers::ignorableWhitespace( const   XMLCh* const chars,
                                   INGXMLSize_t length)
{ }


void UNIMapCompileHandlers::processingInstruction(const  XMLCh* const target
                                            , const XMLCh* const data)
{
    fFormatter << XMLFormatter::NoEscapes << gStartPI  << target;
    if (data)
        fFormatter << chSpace << data;
    fFormatter << XMLFormatter::NoEscapes << gEndPI;
}


void UNIMapCompileHandlers::startDocument() 
{ 
    STATUS 	status;
    i4		tlen = MAX_MAPPINGTBL_SIZE;

    tlen = (tlen + COL_BLOCK) & ~(COL_BLOCK-1);

    map_tbl = (char *)MEreqmem(0, tlen, TRUE, &status);
    if ( status )
      cout<<"UNIMapCompile:ERROR:: Cannot allocate memory for mapping table"<<endl;
    else 
    {
      tptr = map_tbl;

      //    tbl_size_ptr = (SIZE_TYPE *) tptr; 
      //   tptr += sizeof(SIZE_TYPE);
      //   tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

      buffer_size = tlen;
    }
}

void UNIMapCompileHandlers::startElement(const   XMLCh* const    name,
                                AttributeList&	attributes)
{
    u_i2	len;
    i4		index;
    char	charbyte[12];
    char	ucode[10];
    char	ucode_sgt[5];
    char	ru[5];
    char	subchar[5];
    char	subchar1[5]; 
    char	*start;
    STATUS 	status;
    u_i4	byte_result;
    u_i4	uni_result;
    i4 		number = MAX_MAPPINGTBL_SIZE;
    ADU_MAP_HEADER	*map_header;
     u_i4 	sub, sub1;

    if ((buffer_size - (tptr - map_tbl)) < sizeof(ADU_MAP_ASSIGNMENT))
    {
      /* we have reached the threshold for the buffer increase the buffer size */
      char	*map_temp;
      i4	current_bytes = tptr - map_tbl;
		
      buffer_size += MAX_MAPPINGTBL_SIZE;
      buffer_size = (buffer_size + COL_BLOCK) & ~(COL_BLOCK-1);
        
      map_temp = (char *)MEreqmem(0, buffer_size, TRUE, &status);

      if ( status )
        cout<<"UNIMapCompile:ERROR:: Cannot allocate memory for mapping table"<<endl;

      MEcopy(map_tbl, current_bytes, map_temp);
      tptr = map_temp + current_bytes;
      tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));

      /* free the old buffer */
      MEfree(map_tbl);

      map_tbl = map_temp;
      map_header = (ADU_MAP_HEADER *) map_tbl;
      validity_size_ptr = &map_header->validity_size;
      assignment_size_ptr = &map_header->assignment_size;
      subchar_ptr = &map_header->subchar;
      subchar1_ptr = &map_header->subchar1;
    }

    if (!strncmp("characterMapping", StrX(name).localForm(), 16))
    {
      len = attributes.getLength();
      map_header = (ADU_MAP_HEADER *)tptr;

      for (index=0; index < len; index++)
      {
        if (!strncmp("id",(StrX(attributes.getName(index))).localForm(), 2))
          XMLString::copyString(map_header->id,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("version",(StrX(attributes.getName(index))).localForm(), 7))
	XMLString::copyString(map_header->version,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("description",(StrX(attributes.getName(index))).localForm(), 11))
	XMLString::copyString(map_header->description,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("contact",(StrX(attributes.getName(index))).localForm(), 7))
	XMLString::copyString(map_header->contact,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("registrationAuthority",
	    (StrX(attributes.getName(index))).localForm(), 21))
	XMLString::copyString(map_header->registrationAuthority,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("registrationName",
	    (StrX(attributes.getName(index))).localForm(), 16))
	XMLString::copyString(map_header->registrationName,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("copyright",(StrX(attributes.getName(index))).localForm(), 9))
	XMLString::copyString(map_header->copyright,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("bidiOrder",(StrX(attributes.getName(index))).localForm(), 9))
        {
	XMLString::copyString (map_header->bidiOrder,
	    (char *)(StrX(attributes.getValue(index))).localForm());
        }
        else if (!strncmp("combiningOrder",(StrX(attributes.getName(index))).localForm(), 14))
        {
	XMLString::copyString(map_header->combiningOrder,
	    (char *)(StrX(attributes.getValue(index))).localForm());
        }
        else if (!strncmp("normalization",(StrX(attributes.getName(index))).localForm(), 13))
        {
	XMLString::copyString(map_header->normalization, 
	    (char *)(StrX(attributes.getValue(index))).localForm());
        }
      }

      validity_size_ptr = &map_header->validity_size;
      assignment_size_ptr = &map_header->assignment_size;
      subchar_ptr = &map_header->subchar;
      subchar1_ptr = &map_header->subchar1;

      tptr += sizeof(ADU_MAP_HEADER); 
      tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
    }
    else if (!strncmp("state", StrX(name).localForm(), 5))
    {
      ADU_MAP_VALIDITY	map_validity;
      char 		state_start[3] = {' '};
      char		state_end[3] = {' '};
      char		state_max[5] = "FFFF";

      MEfill(sizeof(map_validity),0,&map_validity);
      len = attributes.getLength();
      for (index=0; index < len; index++)
      {
        if (!strncmp("type",(StrX(attributes.getName(index))).localForm(), 4))
	XMLString::copyString(map_validity.state_type,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("next",(StrX(attributes.getName(index))).localForm(), 4))
	XMLString::copyString(map_validity.state_next,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("s",(StrX(attributes.getName(index))).localForm(), 1))
	XMLString::copyString(state_start,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("e",(StrX(attributes.getName(index))).localForm(), 1))
	XMLString::copyString(state_end,
	    (StrX(attributes.getValue(index))).localForm());
        else if (!strncmp("max",(StrX(attributes.getName(index))).localForm(), 3))
	XMLString::copyString(state_max,
	    (StrX(attributes.getValue(index))).localForm());
      }

      status = CVahxl(state_start, &byte_result);
      if(status != OK)
      {
          cout<<"UNIMapCompile:ERROR:: Invalid validity state s= value "<<state_start<<" Status:"<<status<<"\n"<<endl;
      }
      map_validity.state_start = (u_i2)byte_result;

      status = CVahxl(state_end, &byte_result);
      if(status != OK)
      {
          cout<<"UNIMapCompile:ERROR:: Invalid validity state e= value "<<state_end<<" Status:"<<status<<"\n"<<endl;
      }
      map_validity.state_end = (u_i2)byte_result;

      status = CVahxl(state_max, &byte_result);
      if(status != OK)
      {
          cout<<"UNIMapCompile:ERROR:: Invalid validity state max= value "<<state_max<<" Status:"<<status<<"\n"<<endl;
      }
      map_validity.state_max = (u_i2)byte_result;

      tptr = ME_ALIGN_MACRO(tptr, sizeof(u_i2));
      start = tptr;
      MEcopy(&map_validity, sizeof(ADU_MAP_VALIDITY), tptr);
      tptr += sizeof(ADU_MAP_VALIDITY); 
      validity_cnt++;
      *validity_size_ptr += (SIZE_TYPE)(tptr - start);
    }
    else if (!strncmp("assignments", StrX(name).localForm(), 11))
    {
      if (align_validities == FALSE)
      {
        /* Align start of assignment section to PTR and update size */
        /* of validities section if padding is requried            */
        /* Only perform this alignment once.                       */
        start = tptr;
        tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
        *validity_size_ptr += (SIZE_TYPE)(tptr - start);
        align_validities = TRUE;
      }
      len = attributes.getLength();
      for (index=0; index < len; index++)
      {
        if (!strncmp("sub",(StrX(attributes.getName(index))).localForm(), 3))
	{
          XMLString::copyString(subchar,
	      (StrX(attributes.getValue(index))).localForm());
	  STzapblank(subchar, subchar);
	  status = CVahxl (subchar, &sub);
	  if(status != OK)
	  {
              cout<<"UNIMapCompile:ERROR:: Invalid assignment sub= value "<<subchar<<" Status:"<<status<<"\n"<<endl;
	  }
	  *subchar_ptr = sub;
	}
        else if (!strncmp("sub1", (StrX(attributes.getName(index))).localForm(), 4))
	{
          XMLString::copyString(subchar1,
              (StrX(attributes.getValue(index))).localForm());
	  STzapblank(subchar1, subchar1);
	  status = CVahxl (subchar1, &sub1);
	  if(status != OK)
	  {
              cout<<"UNIMapCompile:ERROR:: Invalid assignment sub1= value "<<subchar1<<" Status:"<<status<<"\n"<<endl;
	  }
	  *subchar1_ptr = sub1;
	}
      }
    }
    else if (!strncmp("sub1", StrX(name).localForm(), 4))
    {
      /* FIX ME : Needs to be implemented */
    }	
    else if (!strncmp("a", StrX(name).localForm(), 5))
    {
        len = attributes.getLength();
        for (index=0; index < len; index++)
        {
          /* get b */
          if (!strncmp("b", 
		  (StrX(attributes.getName(index))).localForm(), 1))
          {
            XMLString::copyString(charbyte, 						
                      (StrX(attributes.getValue(index))).localForm());
	    STzapblank(charbyte, charbyte);
	    status = CVuahxl(charbyte, &byte_result);
	    if(status != OK)
	    {
                cout<<"UNIMapCompile:ERROR:: Invalid assignment byte value "<<charbyte<<" Status:"<<status<<"\n"<<endl;
	    }
          }
          /* get u */
          else if (!strncmp("u", 
		  (StrX(attributes.getName(index))).localForm(), 1))
          {
            XMLString::copyString(ucode,
                        (StrX(attributes.getValue(index))).localForm());
	    /* seperate the unicode values, separated by blanks */
	    STzapblank(ucode, ucode);
	    status = CVahxl (ucode, &uni_result);
	    if(status != OK)
	    {
                cout<<"UNIMapCompile:ERROR:: Invalid assignment unicode value "<<ucode<<" Status:"<<status<<"\n"<<endl;
	    }
          }
        }
        ADU_MAP_ASSIGNMENT map_assignment;
        map_assignment.bval = (u_i4) byte_result;
        map_assignment.uval = (u_i2) uni_result;
	map_assignment.uhsur = 0;

        tptr = ME_ALIGN_MACRO(tptr, sizeof(PTR));
        char *start = tptr;
        MEcopy(&map_assignment, sizeof(ADU_MAP_ASSIGNMENT), tptr);
        tptr += sizeof(ADU_MAP_ASSIGNMENT); 
        assignment_cnt++;
        *assignment_size_ptr += (SIZE_TYPE)(tptr - start);
    } /* assignments */
}

STATUS UNIMapCompileHandlers::writedata()
{
    /* create a directory $II_SYSTEM/ingres/files/charmaps */
    CL_ERR_DESC syserr;
	
/* This is how to open the file
    if (CMopen_col("map_header.id", &syserr, CM_UCHARMAPS_LOC) != OK)
    {
	cout <<"MAP COMPILE ERROR: Cannot open alias file for writing \n"<<endl;
	return FAIL;
    }
*/

    i4 tlen = map_tbl_size;
    tlen = (tlen + COL_BLOCK) & ~(COL_BLOCK-1);
    if( CMdump_col (charset, (PTR)map_tbl, tlen, 
				&syserr,  CM_UCHARMAPS_LOC) != OK)
    {
	cout <<"MAP COMPILE ERROR: Cannot write to the mapping file "<<charset<<" \n"<<endl;
	return FAIL;
    }
    return OK;
}
