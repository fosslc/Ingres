/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
 ** Name: SAXImport.cpp - import XML data into an ingres table format.
 **
 ** Description:
 **    This is the main program for the utility impxml. This is used 
 ** to parse any xml file using the xerces parser and then producing
 ** an sql file and data files, which can be uploaded into an Ingres 
 ** database. The metadata in the xml file will be used to create the 
 ** sql file. For each table data defined in the xml file a separate 
 ** data file will be generated, based on the name and the owner of
 ** of the table.
 **  
 ** History:
 **      2-Jan-2001 (gupsh01).
 **	    Created.
 **      2-Jan-2001 (gupsh01).
 **	    Added a call to ios::sync_with_stdio().
 **	19-Sep-2001 (hanch04)
 **	    Removed the passing of the filename to SAXImportHandler
 **	    file i/o is handled through the xf calls
 **	04-Jun-2002 (guphs01)
 **	    set With_65_catalogs variable to true implying that catalog 
 ** 	    level is greater than 65. This is needed in writedefault
 **	    function to write the table defaults correctly.
 **	06-Dec-2002 (gupsh01)
 **	    Added code for closing the sql file opened previously.
 **	14-nov-2003 (abbjo03)
 **	    Due to weird things we do to VMS main programs, we need an extern
 **	    "C" for the main function.
 **	15-mar-2004 (gupsh01)
 ** 	    Modified to support Xerces 2.3.
 **	07-Jul-2004 (hanje04)
 **	    Moved main() so that mkjam/mkming identifies it correctly
 **	22-Oct-2004 (gupsh01)
 **	    Increas the max length of the directory path to accomodate bigger
 ** 	    directory paths.
 **/
/*
**      MKMFIN Hints
**
PROGRAM = impxml

NEEDLIBS =      IMPXMLLIB XFERDBLIB \
                GNLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB \
                COMPATLIB MALLOCLIB SHXERCESLIB

UNDEFS =        II_copyright
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
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include "SAXImport.hpp"

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
static char                    sqlFile[FE_MAXNAME + 1];
static SAXParser::ValSchemes    valScheme       = SAXParser::Val_Auto;



// ---------------------------------------------------------------------------
//  Local helper methods
// ---------------------------------------------------------------------------
static void usage()
{
    cout <<  "\nUsage: impxml [options] file\n"
             "This program prints the data returned by the various SAX\n"
             "handlers for the specified input file. Options are NOT case\n"
             "sensitive.\n\n"
             "Options:\n"
             "    -u=xxx      Handle unrepresentable chars [fail | rep | ref*]\n"
             "    -v=xxx      Validation scheme [always | never | auto*]\n"
             "    -d=xxx      The output directory for the output files\n"
             "    -o=xxx      The output filename for the sql file generated\n"
             "    -n          Enable namespace processing.\n"
             "    -x=XXX      Use a particular encoding for output (LATIN1*).\n"
             "    -?          Show this help\n\n"
             "  * = Default if not provided explicitly\n\n"
             "The parser has intrinsic support for the following encodings:\n"
             "    UTF-8, USASCII, ISO8859-1, UTF-16[BL]E, UCS-4[BL]E,\n"
             "    WINDOWS-1252, IBM1140, IBM037\n"
         <<  endl;
}


#if defined(__VMS)
extern "C" int main(int argC, char* argV[]);
#endif

// ---------------------------------------------------------------------------
//  Program entry point
// ---------------------------------------------------------------------------
int 
main(int argC, char* argV[])
{
    char        *sourcedir = NULL;
    char        *destdir = NULL;
    char        *owner;
    i4 		errCode = 0;

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

    char        directory[MAX_LOC + 1] = {""};

    int parmInd;
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

	   if (STlength(parm) > (MAX_LOC))
	   {
	     cerr << "Error: Directory path is too long, max size allowed: " 
		  << MAX_LOC << endl;  
	     return 2;
	   }
	   STcopy (parm, directory);
	}
       else if(!strncmp(argV[parmInd], "-o=", 3)
              ||  !strncmp(argV[parmInd], "-O=", 3))
        {
           /* set the sql filename */
           const char* const parm = &argV[parmInd][3];
	   STcopy (parm, sqlFile);
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

    xmlFile = argV[parmInd];
    
    if ((STcompare(sqlFile, "") == 0))
    {
	/* 
        ** Make a default sql file based 
	** on the name of xmlfile supplied 
        */
	STcopy (xmlFile, sqlFile);
	strcat(sqlFile,".sql");
    }
	
    if (!With_65_catalogs)
      With_65_catalogs = TRUE;

    /* open the files and set the working directory */
    if (xfsetdirs(directory, sourcedir, destdir) != OK)
        PCexit(FAIL);

    if ((Xf_in = xfopen(sqlFile, 0)) == NULL )
	PCexit (FAIL);

    //
    //  Create a SAX parser object. Then, according to what we were told on
    //  the command line, set it to validate or not.
    //
    SAXParser* parser = new SAXParser;
    parser->setValidationScheme(valScheme);
    parser->setDoNamespaces(doNamespaces);
    /* parser->setDoSchema(doSchema);
       parser->setValidationSchemaFullChecking(schemaFullChecking);
    */

    //
    //  Create the handler object and install it as the document and error
    //  handler for the parser. Then parse the file and catch any exceptions
    //  that propogate out
    //
    try
    {
        SAXImportHandlers handler(encodingName, unRepFlags, NULL);
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
    
    /* close the sql file here */
    if (Xf_in != NULL)
      xfclose(Xf_in);

    return 0;
}
