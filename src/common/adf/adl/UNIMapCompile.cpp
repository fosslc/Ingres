/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
** Name: UNIMapCompile.cpp - compile an XML file into machine readable form
**
** Description:
**    This is the main program for the utility unimapcompile. This is used 
** to parse the mapping files in XML format using the xerces parser.
**  
** History:
**      14-jan-2004 (gupsh01).
**	    Created.
**	29-jan-2004 (gupsh01)
**	    Made -o= option mandatory to avoid overwriting default table 
**	    unintentionaly. 
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
**          in order to avoid compile error in some build environments.
**          Add Ingres header files removed from UNIMapCompileHandlers.hpp.
**      15-Sep-2004 (bonro01)
**          endif can only be followed by a comment on HP.
**	21-Sep-2009 (hanje04)
**	   Use new include method for stream headers on OSX (DARWIN) to
**	   quiet compiler warnings.
**	   Also add version specific definitions of parser funtions to
**	   allow building against Xerces 2.x and 3.x
**/
/*
**      MKMFIN Hints
**
PROGRAM = unimapcompile

NEEDLIBS =     GNLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
               UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
               LIBQGCALIB CUFLIB GCFLIB ADFLIB \
               COMPATLIB MALLOCLIB

UNDEFS =       II_copyright
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

# ifdef min
# undef min
# endif /*min*/
# ifdef max
# undef max
# endif /*max*/

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include "UNIMapCompile.hpp"

// ---------------------------------------------------------------------------
//  Local data
//
//  doNamespaces
//      Indicates whether namespace processing should be enabled or not.
//      Defaults to disabled.
//
//  encodingName
//      The encoding we are to output in. If not set on the command line,
//      then it is defaulted to LATIN1.
//
//  xmlFile
//      The path to the file to parser. Set via command line.
//
//  valScheme
//      Indicates what validation scheme to use. It defaults to 'auto', but
//      can be set via the -v= command.
// ---------------------------------------------------------------------------
static bool                     doNamespaces    = false;
static const char*              encodingName    = "LATIN1";
static XMLFormatter::UnRepFlags unRepFlags      = XMLFormatter::UnRep_CharRef;
static char*                    xmlFile         = 0;
static SAXParser::ValSchemes    valScheme       = SAXParser::Val_Auto;

// ---------------------------------------------------------------------------
//  Local helper methods
// ---------------------------------------------------------------------------
static void usage()
{
    cout <<  "\nUsage: unialscompile [options] file\n"
             "This program prints the data returned by the various SAX\n"
             "handlers for the specified input file. Options are NOT case\n"
             "sensitive.\n\n"
             "Options:\n"
             "    -u=xxx      Handle unrepresentable chars [fail | rep | ref*]\n"
             "    -v=xxx      Validation scheme [always | never | auto*]\n"
             "    -d=xxx      The output directory for the output files\n"
             "    -o=xxx      The output filename for the out file generated\n"
             "    -n          Enable namespace processing.\n"
             "    -x=XXX      Use a particular encoding for output (LATIN1*).\n"
             "    -?          Show this help\n\n"
             "  * = Default if not provided explicitly\n\n"
             "The parser has intrinsic support for the following encodings:\n"
             "    UTF-8, USASCII, ISO8859-1, UTF-16[BL]E, UCS-4[BL]E,\n"
             "    WINDOWS-1252, IBM1140, IBM037\n"
         <<  endl;
}

// ---------------------------------------------------------------------------
//  Program entry point
// ---------------------------------------------------------------------------
int main(int argC, char* argV[])
{
    char outFile[FE_MAXNAME + 1] = {"default"};

    _VOID_ MEadvise(ME_INGRES_ALLOC);

    ios::sync_with_stdio();

    // Initialize the XML4C2 system
    try
    {
         XMLPlatformUtils::Initialize();
    }

    catch (const XMLException& toCatch)
    {
         cerr << "Error during initialization! :\n"
              << StrX(toCatch.getMessage()) << endl;
         return 1;
    }

    // Check command line and extract arguments.
    if (argC < 2)
    {
        usage();
        XMLPlatformUtils::Terminate();
        return 1;
    }

    // Watch for special case help request
    if ((argC == 2) && !strcmp(argV[1], "-?"))
    {
        usage();
        XMLPlatformUtils::Terminate();
        return 2;
    }

    char        directory[FE_MAXNAME + 1] = {""};

    int parmInd;
    bool outFileGiven = false;
    for (parmInd = 1; parmInd < argC; parmInd++)
    {
        // Break out on first parm not starting with a dash
        if (argV[parmInd][0] != '-')
            break;

        if (!strncmp(argV[parmInd], "-v=", 3)
        ||  !strncmp(argV[parmInd], "-V=", 3))
        {
            const char* const parm = &argV[parmInd][3];

            if (!strcmp(parm, "never"))
                valScheme = SAXParser::Val_Never;
            else if (!strcmp(parm, "auto"))
                valScheme = SAXParser::Val_Auto;
            else if (!strcmp(parm, "always"))
                valScheme = SAXParser::Val_Always;
            else
            {
                cerr << "Unknown -v= value: " << parm << endl;
                return 2;
            }
        }
         else if (!strcmp(argV[parmInd], "-n")
              ||  !strcmp(argV[parmInd], "-N"))
        {
            doNamespaces = true;
        }
         else if (!strncmp(argV[parmInd], "-x=", 3)
              ||  !strncmp(argV[parmInd], "-X=", 3))
        {
            // Get out the encoding name
            encodingName = &argV[parmInd][3];
        }
         else if (!strncmp(argV[parmInd], "-u=", 3)
              ||  !strncmp(argV[parmInd], "-U=", 3))
        {
            const char* const parm = &argV[parmInd][3];

            if (!strcmp(parm, "fail"))
                unRepFlags = XMLFormatter::UnRep_Fail;
            else if (!strcmp(parm, "rep"))
                unRepFlags = XMLFormatter::UnRep_Replace;
            else if (!strcmp(parm, "ref"))
                unRepFlags = XMLFormatter::UnRep_CharRef;
            else
            {
                cerr << "Unknown -u= value: " << parm << endl;
                return 2;
            }
        }
	else if(!strncmp(argV[parmInd], "-d=", 3)
              ||  !strncmp(argV[parmInd], "-D=", 3))
	{
           /* set the output directory */
	   const char* const parm = &argV[parmInd][3];
	   STcopy (parm, directory);
	}
       else if(!strncmp(argV[parmInd], "-o=", 3)
              ||  !strncmp(argV[parmInd], "-O=", 3))
        {
           /* set the out filename */
           const char* const parm = &argV[parmInd][3];
	   STcopy (parm, outFile);
    	   outFileGiven = true;
        }
         else
        {
            cerr << "Unknown option '" << argV[parmInd]
                 << "', ignoring it\n" << endl;
        }
    }

    //
    //  And now we have to have only one parameter left and it must be
    //  the file name.
    //
    if (parmInd + 1 != argC)
    {
        usage();
        return 1;
    }

    if (outFileGiven != true)
    {
      cerr << "-o= option is missing, output file name is required please provide a valid output file name\n"
	   << endl;
    }

    xmlFile = argV[parmInd];
    
    //
    //  Create a SAX parser object. Then, according to what we were told on
    //  the command line, set it to validate or not.
    //
    SAXParser *parser = new SAXParser;
    parser->setValidationScheme(valScheme);
    parser->setDoNamespaces(doNamespaces);

    int errCode = 0;
    //
    //  Create the handler object and install it as the document and error
    //  handler for the parser. Then parse the file and catch any exceptions
    //  that propogate out
    //
    try
    {
        UNIMapCompileHandlers handler(encodingName, unRepFlags, NULL, outFile);
        parser->setDocumentHandler(&handler);
        parser->setErrorHandler(&handler);
        parser->parse(xmlFile);
    }

    catch (const XMLException& toCatch)
    {
        cerr << "\nAn error occured\n  Error: "
             << StrX(toCatch.getMessage())
             << "\n" << endl;
        errCode = -1;
    }
    if(errCode) {
        XMLPlatformUtils::Terminate();
        return errCode;
    }

    //
    //  Delete the parser itself.  Must be done prior to calling Terminate, below.
    //
    delete parser;

    // And call the termination method
    XMLPlatformUtils::Terminate();
    
    return 0;
}
