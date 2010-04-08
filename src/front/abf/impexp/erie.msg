/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

/**
** Name:        erie.msg -      iiimport/iiexport message definitions
**
** Description:
**      Defines messages for the iiimport and iiexport, that wacky set
**      of tools which allows users to copy application components around.
**
**      The comment block FOLLOWING each error message gets into the
**      Error Manual, as per the coding standards. To be marginally
**      consistent, comments regarding Message strings also follow the message.
**
** History:
**      25-aug-1993 (daver)
**              First Written.
**      05-oct-1993 (daver)
**              Added E_IE003C_No_param_value message; made a nicer
**              error message for E_IE0013_No_Such_App; changed usage
**              syntax to show -copysrc | -check; added error message
**      26-jan-1994 (daver)
**              Add slow message S_IE0040_No_Allflag_Object and error messages
**              E_IE0041_No_Specified_Object and E_IE0042_Nothing_Found
**              to fix bug 57685 (no error checking for non-existant
**              object in iiexport).
**      21-apr-1994 (connie) Bug #60399
**              Correct spelling for "specified" in S_IE0040_No_Allflag_Object
**      27-oct-1994 (slawrence) Bug #60689
**              Added message E_IE0044_Dupl_File_Name for intfile and listing 
**              command line flags.
**/

# define        _IE_CLASS       217


S_IE0000_Updating_Obj   "Updating %0c `%1c' ...\n"
/*
** ieimport.c 
** Standard message iiimport spews when updating an existing object
** Input:
**      %0c     type of object name (e.g, Form, Frame, Global, etc)
**      %1c     name of the object
*/

S_IE0001_Creating_Obj   "Creating %0c `%1c' ...\n"
/*
** ieimport.c 
** Standard message iiimport spews when creating an existing object
** Input:
**      %0c     type of object name (e.g, Form, Frame, Global, etc)
**      %1c     name of the object
*/

S_IE0003_Writing_file   "Writing components from application `%0c' 
to file `%1c'\n"
/*
** ieemain.c
** iiexport's generic "now this is what I'm about to do" message
** Input:
**      %0c     application name components will come from 
**      %1c     name of the file they will be written to
*/

S_IE0004_Wrote_One      "Wrote %1c `%0c' to the file ..."
/*
** ieexport.sc
** Standard message iiexport spews when writing an object to a file
** Input:
**      %0c     type of object name (e.g, Form, Frame, Global, etc)
**      %1c     name of the object
*/

E_IE0005_Exitmsg        "Processing stops.\n
No changes will be made to the database."
/*
** ieemain.c, ieimain.c, ieutils.sc
** Final message when something's gone wrong. Programs exit after printing.
** NEED TO examine if we wanna use an error or Slow String here.
*/

S_IE0006_Copying_source_files  "Copying source files ...\n"
/*
** iecpysrc.c
** Used to inform that we're starting the process of copying source files
*/

E_IE0007_No_Source_Dir     "No source file directory was specified.\n
The intermediate file does not have a source file directory specified.
This is usually the result of manually editing the intermediate file.
No source files can be copied."
/*%
** Parameters:
**
** Explanation:
**      This error occurs on the rare chance that the application source
**      code directory is missing from the intermediate file. If manually
**      altered, the intermediate file might have been corrupted, thus 
**      preventing IIIMPORT from reading it properly.
**
** System Status:
**      Program exits. No source files are copied, but changes to the
**      database are accepted.
**
** User Action:
**      Examine intermediate file; re-run IIEXPORT to re-generate the file.
*/

E_IE0008_Dir_not_exist "Directory `%0c' does not exist\n.
No files can be copied because the above directory cannot be found by
IIIMPORT. If this intermediate file (and the application components to be
copied) are located on another machine, the source files must be copied
manually."
/*%
** Parameters:
**      %0      Directory name specified in intermediate file.
**
** Explanation:
**      This error occurs if the directory, as specified in the
**      intermediate file, does not appear to exist on the target machine.
**      If the desired source files are located on another machine, they
**      will have to be copied over manually.
** 
** System Status:
**      Program exits. No source files are copied, but changes to the
**      database are accepted.
**
** User Action:
**      Source files must be manually copied.
*/

S_IE0009_Same_Dir "The source code directory specified in the intermediate
file is identical to the source code directory of the target application. 
No copy needs to take place."
/*
** iecpysrc.sc
** Don't have to copy files if they're already in the right place!!
*/

E_IE000A_No_File_Copy   "Unable to open the file `%0c'.\n
IIIMPORT could not open the file `%0c' for copying.  Make sure the file
exists and you have permission to read it."
/*%
** Parameters:
**      %0      Name of the file IIIMPORT is attempting to open
**
** Explanation:
**      This error occurs when IIIMPORT tries to copy source files. The
**      error occurs when trying to read a source file from the directory 
**      specified in the intermediate file. IIIMPORT must be able to read 
**      the file in order to copy it. Make sure you have permission to 
**      read files in the old application's source code directory.
** 
** System Status:
**      Program exits. No source files are copied, but changes to the
**      database are accepted.
**
** User Action:
**      Source files must be manually copied if permissions are
**      not addressed.
*/

E_IE000B_No_File_Create        "Unable to create the %1c file `%0c'.\n
%2c could not create the %1c file `%0c'.  Make sure that the file 
name is valid and that you have permission to create the file."
/*%
** Parameters:
**      %0      Name of the file program is attempting to create
**      %1      Type of file: "listing", "temporary", or "intermediate".
**      %2      Which program issues the message (IIIMPORT or IIEXPORT).
**
** Explanation:
**      This error occurs in IIMPORT when either creating the listing file, 
**      or when copying source files. If the file name is "iiIEsrcX.tmp",
**      it means IIIMPORT could not open a file in the existing
**      application's source code directory in preparation for copying
**      source files. 
**      
**      This error occurs in IIEXPORT when creating the intermediate file. 
**      Make sure that you have permission to write in this directory.
** 
** System Status:
**      Program exits. For IIIMPORT, no source files are copied, but 
**      changes to the database are accepted. For IIEXPORT, processing stops. 
**
** User Action:
**      For IIIMPORT, source files must be manually copied if write 
**      permissions are not addressed. IIEXPORT users must assure they
**      have write permissions in the target directory.
*/


E_IE000C_EXCEPTION      "INTERNAL ERROR: Could not set exception handler.\n
An internal error in %0c occurred.  %0c could not set an exception
handler that it uses.  Retry running %0c and if the problem persists
notify your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of executable: either IIIMPORT or IIEXPORT
**
** Explanation:
**      This error occurs when an error status comes back from an
**      internal function. 
** 
** System Status:
**      Fails. 
**
** User Action:
**      Retry; call Tech Support as instructed.
*/


E_IE000D_Need_2_Args    "\nSyntax Error: IIIMPORT requires a database name and
application name."
/*%
** Parameters:
**      None
**
** Explanation:
**      This error occurs when the user supplies 0 or 1 parameters.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/


E_IE000E_Flags_Before_DB_App    "\nSyntax Error: Flags were issued before
the database or application name."
/*%
** Parameters:
**      None
**
** Explanation:
**      This error occurs when a flag (something which begins with `-')
**      is detected where the database name or application name is expected.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/


E_IE000F_Args_have_dash "\nSyntax Error: Flags must begin with `-'.\n
Flag %0c is illegal."
/*%
** Parameters:
**      %0      The illegal flag issued.
**
** Explanation:
**      This error occurs when a flag is issued without a leading `-'.
**      All flags must be preceded by `-'.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/

E_IE0010_Illegal_flag   "\nSyntax Error: Illegal Flag found.\n
Flag %0c is unknown to %1c.\n"
/*%
** Parameters:
**      %0      The illegal flag issued.
**      %1      The name of the program running (IIIMPORT or IIEXPORT).
**
** Explanation:
**      This error occurs when the user issues an flag unknown to 
**      the program currently being executed.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/


E_IE0011_No_equals_used "\nSyntax Error: No `=' found with flag %0c\n."
/*%
** Parameters:
**      %0      The illegal flag issued.
**
** Explanation:
**      The correct syntax for flag use is "flag=item,item,item...".
**      No `=' was found when processing the mentioned flag.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/

E_IE0012_Need_3_Args    "\nSyntax Error: IIEXPORT requires a database name,
application name, and something to export."
/*%
** Parameters:
**      None
**
** Explanation:
**      This error occurs when the user supplies less than 3 parameters.
**
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/

E_IE0013_No_Such_App    "The application `%0c' does not exist in the
specified database. Make sure you have not mis-spelled `%0c',
and that you specified the appropriate database name.\n"
/*%
** Parameters:
**      %0      Name of the application on the command line
**
** Explanation:
**      This error occurs when the user mis-types or specifies an
**      application which does not exist for the given database.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue command using correct application name.
*/

E_IE0014_Bad_CatGet        "Cannot select objects for application `%0c'.\n
The objects for the application `%0c' could not be retrieved from the
database.  An internal module that retrieves them returned a value of `%1x'."
/*%
** Parameters:
**      %0      Name of the application in question
**      %1      Error number returned by the internal module
**
** Explanation:
**      There was an error retrieving the objects for an application.
**      This could indicate an internal problem in IIEXPORT, but could
**      also exist because of problems in the database.
**
** System Status:
**      Program issues an error message, and exits.
**
** User Action:
**      Make sure the application name is correct.  Try running
**      ABF on the application.  Finally, report the error to
**      your Site Technical Contact.  Make sure you
**      report value given in the error.
*/

E_IE0015_Err_LongRemark  "Cannot read long remarks for object.\n
The long remarks for the object `%1c,' which is part of the application,
`%0c', could not be read from the catalogs.  Please report this problem
to your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of the application objects belongs to
**      %1      Name of the object whose long remarks we couldn't read
**
** Explanation:
**      This error occurs when the long remarks cannot be retrieved out
**      of the system catalogs for the given object. This indicates that 
**      the system catalogs have been altered manually (or have in other ways
**      been corrupted). 
** 
** System Status:
**      Program issues an error message, and exits.
**
** User Action:
**      Contact your Site Technical Contact. Try running ABF on the
**      application; see if an error occurs when "MoreInfo" is selected.
*/

E_IE0016_No_Outfunc      "INTERNAL ERROR: No output function for object\n.
The object `%0c' of class `%1d' does not have an output function defined
for it.  Processing will continue, but this object will not be written
to the file. This is an internal problem in IIEXPORT and should be
reported to your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of the application object
**      %1      Class Id of the object.
**
** Explanation:
**      There is an internal error in IIEXPORT so that it can not
**      copy out an object. Generally, an output function is class
**      specific, which is why the (usually internal) Class ID of the 
**      object is printed.
**
** System Status:
**      IIEXPORT will simply ignore this object and continue to
**      write the output file.
**
** User Action:
**      Report this to your Site Technical Contact; the class id and
**      object name will be important to note.
*/

E_IE0017_No_Class_Thing "Could not copy %1c `%0c' to the file"
/*%
** Parameters:
**      %0      Name of the object in question
**      %1      Type of the object (Form, Frame, etc)
**
** Explanation:
**      This error occurs when the output function returns an error. 
** 
** System Status:
**      Processing continues, but IIEXPORT could not write the object.
**
** User Action:
**      Report this to your Site Technical Contact.
*/

E_IE0018_Unknown_Class  "INTERNAL ERROR: Unknown class found.\n
IIEXPORT was attempting to process an object of class `%0d'. This class
is unknown to the internals of IIEXPORT. Processing will continue,
but this is an internal problem in IIEXPORT and should be reported 
to your Site Technical Contact."
/*%
** Parameters:
**      %0      Unknown Class found
**
** Explanation:
**      A class unknown to an internal routine was passed to it. This is
**      an internal error, otherwise known as an "impossible condition" 
**      check. It occurred in the section of code which processed the
**      -allflags. It should be reported to the Site Technical Contact.
** 
** System Status:
**      Processing continues after the message. The current object will
**      not be included by the -all<flag> mechanism (e.g., if it is a frame, 
**      and -allframes was issued, this frame will be skipped).
**
** User Action:
**      Report this to your Site Technical Contact.
*/

S_IE0019_Deleting_File  "Removing file `%0c' ... \n"
/*
** ieimain.c
** iiimport's message when dropping the intermediate file
** Input:
**      %0c     name of the intermediate file
*/

E_IE001A_Memory_Alloc_Err "INTERNAL ERROR: Memory allocation.
An internal error has occurred in %0c. Retry running %0c, 
and if the problem persists notify your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of executable: either IIIMPORT or IIEXPORT
**
** Explanation:
**      This is an internal error during a memory allocation phase.
** 
** System Status:
**      Processing stops.
**
** User Action:
**      Notify the Site Technical Contact if it persists.
*/

S_IE001B_Correct_Syntax_Is      "The Correct Syntax is:\n"

S_IE001C_IIExport_Usage "        iiexport <dbname> <appname> <flags>\n
\nwhere <flags> are:\n\n
        -allframes    | -frame=name,name,name,...\n
        -allprocs     | -proc=name,name,name,...\n
        -allrecords   | -record=name,name,name,..."

S_IE003C_IIExport_Usage2 "         -allglobals   | -global=name,name,name,...\n
        -allconstants | -constant=name,name,name,...\n
        -intfile=intfilename\n
        -listing=filename\n
        -user=username\n\n"

S_IE001D_IIImport_Usage "        iiimport <dbname> <appname> [<flags>]\n\n
where <flags> are:\n\n
        -copysrc | -check\n
        -dropfile"

S_IE003D_IIImport_Usage2 "         -user=username\n
        -listing=filename\n
        -intfile=filename\n\n"

/*%
** Import and Export usage messages
*/


E_IE001E_Multiple_Obj_Found     "INTERNAL CONSISTENCY ERROR: More than 
one object found.\n  There is more than one entry corresponding to 
the %0c `%1c'. This indicates that the system catalogs may be corrupt.
Please notify your Site Technical Contact."
/*%
** Parameters:
**      %0      Type of object it is
**      %1      Name of the object.
**
** Explanation:
**      This error occurs when more than one row has been found
**      corresponding to a particular object in catalogs where only one 
**      row per object is allowed. The system catalogs may have been 
**      manually altered, or may be corrupt.
** 
** System Status:
**      Processing stops.
**
** User Action:
**      Notify the Site Technical Contact.
*/

E_IE001F_No_Source_Line "Error in intermediate file: bad source line for
application found.\nIIIMPORT expects that the in the file containing the
source code directory be in the proper format. This indicates the 
intermediate file has been altered. Please re-run iiexport to re-generate 
the intermediate file."
/*%
** Parameters:
**      None
**
** Explanation:
**      An internal check discovered that the third line in the file, 
**      immediate following the OC_APPL: line, is corrupt. This usually means
**      the file has been altered.
**
** System Status:
**      Processing Stops; the program exits.
**
** User Action:
**      Re-run iiexport to re-generate the file. 
*/

E_IE0020_Cannot_Read_File "Can't open file `%0c' for reading.\n"
/*%
** Parameters:
**      %0      Name of the file
**
** Explanation:
**      The file couldn't be open to process it.
**
** System Status:
**      Processing Stops; the program exits.
**
** User Action:
**      Make sure that the file exists and is readable by you.
*/


E_IE0021_Bad_Format     "The intermediate file has an invalid format.\n
An error was discovered in the first line of the file '%0c'. Please check the
file and, if necessary, use IIEXPORT to create a new intermediate file."
/*%
** Parameters:
**      %0      Name of the file
**
** Explanation:
**      IIIMPORT has discovered, while attempting to copy in, that the first
**      line of the intermediate file has a bad format.
**
** System Status:
**      Processing Stops; the program exits.
**
** User Action:
**      Check the intermediate file to see whether it really is an 
**      intermediate file, and, if necessary, run IIEXPORT again to
**      create a new one. Assure that the file has not been manually 
**      edited.
*/

E_IE0022_Not_Compatible "Incompatible file version.\n
The version of the file `%0c' is later than this version of IIIMPORT,,
which is not upwardly compatible.  The file cannot be copied into the
database."
/*%
** Parameters:
**      %0      Name of the file
**
** Explanation:
**      The version of the file is from a release later than that of the 
**      IIIMPORT utility being used to import it into the database.  
**      Since the file contains data for features not yet supported by 
**      this release that includes this IIIMPORT utility, it cannot be 
**      copied into the database.
**
** System Status:
**      IIIMPORT will terminate without importing the objects in the file.
**
** User Action:
**      Your system administrator must upgrade your INGRES release to the
**      version compatible with the system from which the file was copied.
**      Please contact your INGRES system administrator.
*/

E_IE0023_Out_Of_Order   "IIIMPORT expected to read the first line of the next
object to be processed, but didn't find one. Processing will stop, and all
objects previously read in will be backed out. Please make sure that this file 
has not been manually edited. Re-run IIEXPORT to recreate the intermediate 
file, and inform your Site Technical Contact if it persists."
/*%
** Parameters:
**      None
**
** Explanation:
**      IIIMPORT uses a state machine to read in the file. At this point, it
**      expects the beginning of a new object definition (a line beginning with
**      "OC_XXX:", where XXX indicates which object it is). Instead, it found
**      a line beginning with a tab or CUC_XXX. This indicates that the file
**      has been hand edited. 
** 
** System Status:
**      Processing Stops. No changes will be made to the database.
**
** User Action:
**      Regenerate the file; contact your Site Technical Contact if it
**      persists. 
*/

S_IE0024_Check_Found    "IIIMPORT Check flag: Successfully found %1c `%0c'.\n"
/*
** Parameters:
**      %0      Object name
**      %1      Type of object (frame, form, whatever)
**
** Explanation:
**      The -check flag prints this message for each object it finds.
** 
** System Status:
**      Keep finding objects. Everything a-ok.
**
** User Action:
**      Keep watching!
*/

S_IE0025_No_Check_Found "IIIMPORT Check flag: DID NOT FIND %1c `%0c'.\n"
/*
** Parameters:
**      %0      Object name
**      %1      Type of object (frame, form, whatever)
**
** Explanation:
**      The -check flag prints this message for each object it can't find.
** 
** System Status:
**      Keep trying to find objects.
**
** User Action:
**      Keep watching!
*/

E_IE0026_Unexpected_EOF "Unexpected End Of File found.\n
The end of the intermediate file was reached while reading the 
definition for `%0c'.  Please make sure the intermediate file was not 
manually edited, and re-run IIEXPORT to generate a new intermediate file."
/*%
** Parameters:
**      %0      Name of the object being processed when error occurred.
**
** Explanation:
**      IIIMPORT ran into the end of the file while processing an object. This
**      either indicates an internal error, or that the file was manually 
**      edited.
** 
** System Status:
**      Processing Stops. No changes are made.
**
** User Action:
**      Re-generate the file via IIEXPORT. Notify the Site Technical Contact
**      if the problem persists.
*/

E_IE0027_Not_Data_Line  "Error in reading intermediate file:\n
IIIMPORT expected to read a detail line (which begins with a tab) while
processing object `%0c'. The internal code returned was `%1d'. Please
make sure the intermediate file was not manually edited, and re-run 
IIEXPORT to generate a new intermediate file."
/*%
** Parameters:
**      %0      Name of object being processed when error occurred.
**      %1      Internal object code returned.
**
** Explanation:
**      This error occurs when IIIMPORT expect to be reading a data line.
**      These lines begin with a tab. The first token read in was not a tab,
**      triggering this error. Make sure the file has not been manually
**      edited.
** 
** System Status:
**      Processing Stops. No changes are made.
**
** User Action:
**      Re-generate the file via IIEXPORT. Notify the Site Technical Contact
**      if the problem persists.
*/

E_IE0028_Unknown_Record_Type    "Unknown object class found.\n
An internal object class of `%1d' was defined for object `%0c'.
This object class is undefined for this release of IIIMPORT. If 
possible, regenerate the intermediate file via IIEXPORT, and  
notify your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of object being processed when error occurred.
**      %1      Internal object code returned.
**
** Explanation:
**      IIIMPORT understands specific object classes. An object was found 
**      in the file with an object class unknown to IIIMPORT. 
** 
** System Status:
**      Processing Stops. No changes are made.
**
** User Action:
**      Assure the file has not been manually edited, regenerate the
**      file via IIEXPORT, and notify the Site Technical Contact if the
**      problem persists.
*/


E_IE0029_Unknown_Component      "Unknown component type found.\n
An internal component type of `%1d' was found while processing 
object `%0c'.  This component type is undefined for this release of 
IIIMPORT. If possible, regenerate the intermediate file via IIEXPORT, 
and notify your Site Technical Contact."
/*%
** Parameters:
**      %0      Name of object being processed when error occurred.
**      %1      Internal component type returned.
**
** Explanation:
**      IIIMPORT understands specific component types. An object was found 
**      in the file with a component type class unknown to IIIMPORT. 
** 
** System Status:
**      Processing Stops. No changes are made.
**
** User Action:
**      Assure the file has not been manually edited, regenerate the
**      file via IIEXPORT, and notify the Site Technical Contact if the
**      problem persists.
*/

E_IE003A_DBMS_Error     "An error occurred while writing the data in the
intermediate file to the DBMS catalogs.\n"
/*%
** Parameters:
**      None
**
** Explanation:
**      This error occurs when one of the database writing routines returns
**      with an error status. All previous objects read in will be backed
**      out.
** 
** System Status:
**      Processing Stops. No changes will be made. 
**
** User Action:
**      Ensure the Database Server is functioning properly.
**      Notify the Site Technical Contact.
*/

E_IE003B_Joindef_Error  "An error occurred while reading the definition for
Joindef `%0c'. At the end of the Joindef definition, a component token was
found. IIIMPORT expected to find another object definition, or End Of File.
Please make sure that the intermediate file has not been manually edited,
regenerate the file via IIEXPORT, and notify your Site Technical Contact if
the problem persists."
/*%
** Parameters:
**      %0      Joindef name.
**
** Explanation:
**      This error occurs in an "impossible condition" check internally.
**      This should never occur unless the file has been altered in some
**      manner, or if IIEXPORT is malfunctioning.
** 
** System Status:
**      Processing stops. No changes will be made.
**
** User Action:
**      Assure the file has not been manually edited, regenerate the
**      file via IIEXPORT, and notify the Site Technical Contact if the
**      problem persists.
*/

/* messages 3C and 3D are usage messages */

E_IE003E_No_param_value "\nSyntax Error: No parameter value found for
flag `%0c'. This flag requires a value.\n"
/*%
** Parameters:
**      %0      The flag issued without a value.
**
** Explanation:
**      The correct syntax for flag use is "flag=item,item,item...".
**      No item(s) were found when processing the mentioned flag.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/

E_IE003F_Both_Check_Copysrc     "\nSyntax Error: Both -copysrc and
-check flags were specified, which is illegal. Use -check to verify
existence of application components, and -copysrc to move source code
files importing application components.\n"
/*%
** Parameters:
**      None
**
** Explanation:
**      The -check and -copysrc flags are mutually exclusive.
** 
** System Status:
**      Program issues a usage message, and exits.
**
** User Action:
**      Re-issue using correct syntax.
*/

S_IE0040_No_Allflag_Object      "\nThe `%0c' flag was specified on the 
command line.\nNo %1c were found for the specified application.\n"
/*%
** Parameters:
**      %0      flag name (e.g., -allprocs, -allframes, etc)
**      %1      object type name (e.g., procedures, frames, etc)
**
** Explanation:
**      This message is printed when the user specifies an "-all" flag,
**      and no objects of that type are found for the given application. 
**      For example, if the user specified -allrecords and there were no
**      record types defined for this application, this message would be
**      printed.
** 
** System Status:
**      Notify the user. They should know that the tool found none of
**      these objects in the application.
**
** User Action:
**      None, unless user expects the application contains an object of
**      this type.
*/

E_IE0041_No_Specified_Object "IIEXPORT cannot find `%0c' in this
application as specified.  Make sure that `%0c' is correctly spelled, 
and that it is of the correct object type."
/*%
** Parameters:
**      %0      component name 
**
** Explanation:
**      This error occurs when IIEXPORT cannot find the given object on
**      the command line. It usually means the object name is
**      misspelled, or perhaps that the user specified it as a frame
**      when it really is a procedure.
** 
** System Status:
**      Notify the user. They should know that the tool did not find
**      this object in the application.
**
** User Action:
**      Reissue, specifying the correct spelling or component type.
*/

E_IE0042_Nothing_Found "IIEXPORT found nothing to export. None of the 
objects specified on the command line were found for this application."
/*%
** Parameters:
**      None
**
** Explanation:
**      Nothing was found to export. Either the no objects were
**      specified, there were none of that component type for this 
**      application, or a specific object was misspelled.
** 
** System Status:
**      Notify the user. The system will exist, and delete the empty
**      iiexport intermediate file.
**
** User Action:
**      Reissue, specifying valid objects to export.
*/


E_IE0043_FrameProc_Name "Error importing %2c `%1c'. IIMPORT has detected
that an object `%1c' (a %0c) already exists for this application, but its
type differs from the object described in the intermediate file. Keep in
mind that IIIMPORT cannot be used to change the type of an existing 
object (e.g, an update frame to a browse frame or a 3GL procedure to a 
4GL procedure). Please rename either Frame or Procedure, and re-run 
IIEXPORT and IIIMPORT."
/*%
** Parameters:
**      %0      Type (Frame or Procedure) being IMPORTED into the application.
**      %1      Name of the object.
**      %2      Type (Frame or Procedure) existing in the application.
**
** Explanation:
**      The user is trying to import a frame or procedure of the same
**      name, but a different type (user class). This can happen because 
**      frames and procedures have different types (e.g, browse, update, 
**      append, menu, oslproc, c proc, etc) but share the same name space.
**      If the user is trying to change the type of the object, mention
**      that that is illegal.
** 
** System Status:
**      Processing stops.
**
** User Action:
**      Rename one of the two objects, and re-run IIEXPORT and IIIMPORT.
*/

E_IE0044_Dupl_File_Name "The -intfile filename  and  the -listing filename
must be unique.  Please supply unique file names for each of these flag  
variables."
/*%
** Parameters:
**      none
**
** Explanation:
**      The user is trying to import or export an application and    
**      has supplied duplicate file names for the intfile and listing    
**      flags. Some platforms will not permit the opening of files with  
**      identical names by the same process.
** 
** System Status:
**      Processing stops.
**
** User Action:
**      Provide unique file names, and re-run IIEXPORT or IIIMPORT.
*/
