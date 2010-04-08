/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ercu.msg -	Copy Utility Message Definitions File.
**
** History:
**	11-aug-1987 (Joe)
**		Initial Version
**	19-sep-1989 (Kathryn) Bug 7507
**		Corrected E_CU0002 from "open open file" to "open file".
**	02-nov-1989 (marian)
**		Replace "Technical Support Representative" with "Site
**		Technical Contact".
**	90/03  wong
**		Added "not compatible" error, E_CU000A.
**	19-jun-91 (blaise)
**		Added new message S_CU0014 to warn users that we will not
**		attempt to copy a vision-specific system catalog to an ABF-
**		only environment. Bug #37986.
**	24-jun-91 (blaise)
**		Changed the wording of S_CU0014 to be less technical (it now
**		refers to "Vision-specific information" rather than system
**		catalogs.
**	7-jan-93 (blaise)
**		Added E_CU0013_BAD_FORMAT for intermediate files with the
**		wrong format; and E_CU0015_OLD_VERSION for pre-6.0 intermediate
**		files - from 6.5 we no longer attempt to convert these.
**	26-jul-93 (blaise)
**		Added F_CU0014_CLASSID to replace the literal "Classid" in
**		cuencode.qsc.
**      13-Jul-2009 (hanal04) Bug 122276
**              Add the tmp buffer mentioned in the change for Bug 121512.
**              A client has hit the case where the new ID has more digits and
**              retesting 4gl69.sep is not showing any difs.
**/

# define	_CU_CLASS	182

E_CU0001_UNKNOWN_CLASS	"Internal Error: Unknown record type.\n
The routine `%1c' was passed an unknown record type `%0d.'"
/*%
** Explanation:
**	An illegal record type was some how generated
**	in the copy frontend object utility.  This is
**	an internal error and should be reported.
** System Status:
**	This particular object will be ignored, but processing
**	will continue.
** User Action:
**	Report this problem.  Save the input file to
**	document the problem.
*/

E_CU0002_CANNOT_READ_FILE "Can't open file `%0c' for reading.\n"
/*%
** Explanation:
**    	The file couldn't be open to process it.
** System Status:
**   	 The problem will terminate.
** User Action:
**	Make sure that the file exists and is readable by you.
*/

E_CU0003_NO_GBF	"No GBF in this database.\n
The file contained a GBF graph.  This database does not support
GBF"
/*%
** Explanation:
**	As of 6.0, GBF is no longer a supported product.  Only
**	some pre 6.0 database have the catalogs needed to support
**	GBF.  In this case, the input file contained a GBF graph,
**	but no GBF catalogs exist.
** System Status:
**	The GBF graph will be ignored.
** User Action:
**	If you think you should have GBF, check with your
**	Site Technical Contact.
*/

E_CU0004_CANNOT_CONVERT_FILE "Cannot convert file to 6.0 format.\n
An error occurred in converting the input file `%0c' to the 6.0 file `%1c'.  The
file cannot be copied to the database."

/*%
** Explanation:
**	There was an error converting the input file to 6.0 format.
**	An earlier message should should have give a reason
**	why the conversion failed.
** System Status:
**	The program will terminate.
** User Action:
**	Correct the earlier error.  If no error was given, save
**	the input file and the output file named in the error
**	and notify your Site Technical Contact.
*/

E_CU0005_NO_CONVERT_FUNCTION "Cannot convert file to 6.0 format.\n
This copy utility does not have a conversion function to convert the input file
to the proper format.  The file cannot be copied into the database."

/*%
** Explanation:
**	The input file is not of the correct format for this version
**	of the program and it does not have a function to convert
**	it.  This is either an internal problem in the program
**	or it is a bad file.
** System Status:
**	The program will terminate.
** User Action:
**	Make sure the file is the proper format for the program
**	begin run.
*/

S_CU0006_GOOD_COPY "Copied %0c `%1c' to the database."
/*
** The first argument is the type of object being copied,
** (It will be one of the fast strings below)
** the second argument is the name of the object.  This
** message is informational to tell the user the object
** was copied.
*/

S_CU0007_BAD_COPY "Could not copy %0c `%1c' to the database."
/*
** The first argument is the type of object being copied,
** (It will be one of the fast strings below)
** the second argument is the name of the object.  This
** message is informational to tell the user the object
** wasn't copied.
*/

E_CU0008_OUT_OF_ORDER "The copy file contains a bad line.\n
A level number is out of order.  Please save the copy file and
notify your Site Technical Contact.  This object
will not be copied.  Bad line for object `%0c' with class number `%1d'"
/*%
** Explanation:
**	The input file had a bad line in it.  This usually represents
**	a problem in the copy program.
** System Status:
**	The program will not copy this object.
** User Action:
**	Save the input file so that a Site Technical Contact
**	can diagnosis the problem.
*/

E_CU0009_NO_SUCH_GRAPH "Graph `%0c' not found.\n
The graph named `%0c' could not be found to copy out."
/*%
** Explanation:
**	The graph could not be found to copy out.
** System Status:
**	The program will not copy this object.
** User Action:
**	Enter the name of a correct graph.
*/

E_CU000A_NOT_COMPATIBLE	"Incompatible file version.\n
The version of the file `%0c' is later than this version of the copy utility,
which is not upwards compatible.  The file cannot be copied into the database."

/*%
** Explanation:
**	The version of the file is from a release later than that of the copy
**	utility being used to copy it into the database.  Since the file
**	contains data for features not yet supported by this release that
**	includes this copy utility, it cannot be copied into the database.
**
** System Status:
**	The copy utility will terminate without copying the file.
**
** User Action:
**	Your system administrator must upgrade your INGRES release to the
**	version compatible with the system from which the file was copied.
**	Please contact your system administrator.
*/

E_CU0010_BAD_ENCODE "The entry  in the file for `%1c' `%0c' is bad.\n
The lines for an encoded object in the copy file were bad.  The
class of the object was `%1d' the name was `%0c'.  Please save
the copy file and report this to your Site Technical Contact."
/*%
** Explanation:
**	The copy input file contain a bad line.
** System Status:
**	The program will ignore this object and try to
**	proceed.
** User Action:
**	Save the copy file and report the problem to your
**	Site Technical Contact.
*/

E_CU0011_BAD_SAVE	"Could not save the `%1c' `%0c'.\n
The `%1c' `%0c' could not be saved in the database."
/*%
** Explanation:
**	An encoded object was trying to be saved in the database
**	but couldn't.  A previous error should give the reason why.
**	If not, report this to your Site Technical Contact.
** System Status:
**	The program will ignore this object and try to proceed.
** User Action:
**	If no previous error was given explaining why the save failed,
**	please report this to your Site Technical Contact.
*/

E_CU0012_BAD_ENCODE_END "The entry in the file for `%1c' `%0c' is bad.\n
The entry for the encoded `%1c' object `%0c' is bad.  The end marker
was `%2c'."
/*%
** Explanation:
**	The copy input file contain a bad line.
** System Status:
**	The program will ignore this object and try to
**	proceed.
** User Action:
**	Save the copy file and report the problem to your
**	Site Technical Contact.
*/

/*
** These are the types of objects.  This strings will
** get used in messages like those above.
*/
F_CU0001_GBF_GRAPH	"GBF Graph"
F_CU0002_APPLICATION	"Application"
F_CU0003_PROCEDURE	"Procedure"
F_CU0004_QBFNAME	"QBFName"
F_CU0005_FRAME		"Frame"
F_CU0006_FORM		"Form"
F_CU0007_JOINDEF	"Joindef"
F_CU0008_REPORT		"Report"
F_CU0009_GRAPH		"Graph"
F_CU0010_CLASS		"Record Type"
F_CU0011_ATTRIBUTE	"Record Attribute"
F_CU0012_GLOBAL		"Global Variable"
F_CU0013_CONSTANT	"Application Constant"

/* used by cuencode.qsc for unknown class: */
F_CU0014_CLASSID	"Classid"

S_CU0014_VISION_CATALOG "Warning: The copy file contains Vision-specific information.\n
This information will be ignored since it cannot be copied to an ABF-only
environment."
/*%
** Explanation:
**	Copyapp has encountered a Vision system catalog while copying an
**	application into an ABF-only environment.
** System Status:
**	The program will not attempt to copy this table into the destination
**	database and will move on to the next line of the intermediate file.
** User Action:
**	If you think the destination database should contain the Vision
**	catalogs, report the problem to your site technical contact. Otherwise
**	no action is necessary.
*/

E_CU0013_BAD_FORMAT "The intermediate file has an invalid format.\n
An error was discovered in the first line of the file '%0c'. Please check the
file and, if necessary, create a new intermediate file with 'copyapp out'."
/*%
** Explanation:
**	Copyapp has discovered, while attempting to copy in, that the first
**	line of the intermediate file has a bad format.
** System Status:
**	The program will exit.
** User Action:
**	Check the intermediate file to see whether it really is a copyapp
**	intermediate file, and, if necessary, run 'copyapp out' again to
**	create a new one.
*/

E_CU0015_OLD_VERSION "Incompatible file version.\n
The version of the file '%0c' is older than 6.0, and is not supported by
the current release of the copy utility. Please check with your Site
Technical Contact for information on how to convert this file to a version
compatible with the current release."
/*%
** Explanation:
**	The version of the copyapp intermediate file is 5.0 or earlier.
**	From 6.5, we no longer attempt to convert old files to 6.0 format.
** System Status:
**	The program will exit.
** User Action:
**	Running an earlier version of copyapp (6.0-6.4) will convert the
**	intermediate file and copy the application into the system catalogs.
*/

E_CU0016_CNVT_TO_QRY_ERR "Fatal error trying to convert a file line to query."
/*%
** Explanation:
**      IICUltdLineToDescriptor() does not have space in the input
**      buffer to retokenise the input buffer with the new Object ID.
** System Status:
**      The program will exit.
** User Action:
**      Retry in a fresh database. Failing that we must have a LOT of
**      objects and long Object IDs.
*/
