/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erde.msg -	Message file for class DE
**
** Description:
**	This file defines the messages for class DE.
**
** History:
**	18-nov-1987 (peter)
**		Created.
**	2-dec-1992 (rdrane)
**		Added many additional messages since this file
**		will now be shared with a production version
**		of the utility.
**	12-jan-93 (rdrane)
**		Rename msg text since we finally seem to have decided on
**		a name for the production version of the utility.
**	6-apr-93 (rdrane)
**		Add message for erroneous multiple object type specification
**		(E_DE0015_Dobj_multype).
**/

# define	_DE_CLASS	80

E_DE0001_NoClass	"No object type '%0c' found.\n
No object type '%0c' found.  Valid values are 'a',
'f', 'g', 'j', 'r', or 'q'.  This parameter will be
skipped."
/*%
** Explanation:
**	This occurs if a bad prefix is given on the command line or
**	in the prompt, and no matching class of frontend objects
**	corresponds to the prefix.
**
** System Status:
**	This parameter will be skipped.
**
** User Action:
**	Specify the correct prefix and rerun command.
*/

E_DE0002_NotDBA "Only the DBA may use the -c flag.\n
You are not the DBA for this database, so you cannot
use the -c flag."
/*%
** Explanation:
**	The -c flag is used to delete all objects, irrespective of
**	who owns it.  This is only allowed by the DBA.
**
** System Status:
**	Program stops.
**
** User Action:
**	Use the -u flag to become the DBA, if you have authority.
*/

E_DE0003_BadFlag	"Bad flag %0c specified.\n
The correct syntax is:
\n
\n    deleter [-s] [-uuser] [-c] dbname
\n        {-[rfjagq]objectname} "
/*%
** Explanation:
**	An illegal flag was specified.
**
** System Status:
**	Program stops.
**
** User Action:
**	Correct error and retry.
*/

F_DE0001_Database	"Database?"		/* prompt */

E_DE0004_NoOpen	"Could not open database.\n
An error occurred in opening the database.  Details were in
the previous message."
/*%
** Explanation:
**	An error occurred, and was displayed to the user.
**
** System Status:
**	Program aborts.
**
** User Action:
**	Try again.
*/

F_DE0002_Object	"Object to Delete (end list with RETURN)"
		/* Prompt given if no command line args specified */


E_DE0005_BadType	"Bad object type %0c given.\n
A bad object type prefix was found.  Valid prefixes are
'j' 'a' 'f' 'r' 'q' and 'g'.  Reenter."
/*%
** Explanation:
**	Prefix did not match one of the valid prefixes.
**
** System Status:
**	Reprompt.
**
** User Action:
**	Reenter correct entry.
*/

E_DE0006_TooMany	"More than %0d matching objects found.\n
More than %0d matching objects were found when checking for
type %1c with name of %2c and owner %3c.  Any more than %0d will be left in
the database."
/*%
** Explanation:
**	Only the first set will be processed.
**
** System Status:
**	The rest will be left in the database.
**
** User Action:
**	Rerun command to get the rest.
*/

/*
**	The following are used in the messages in deleter
**	to say what is being acted upon
*/

F_DE0003_App	"application"
F_DE0004_Form	"form"
F_DE0005_Graph	"graph"
F_DE0006_Jdef	"joindef"
F_DE0007_Qbfnm	"qbfname"
F_DE0008_Rep	"report"
F_DE0009_Dobj_O	"object"

S_DE0007_Status "Deleting %0c %1c with owner %2c from database..."
	/* Status message for what is happening */

S_DE0008_NoMatch "No %0cs matching name %1c and owner %2c found..."
	/* Status if none found */

E_DE0009_GenError "Error occurred in deleting %0c.\n
An error occurred in deleting %0c %1c with owner %2c.  The status
code is %3x.  A previous message should explain what the problem
was."
/*%
** Explanation:
**	An error status was returned from the deleting routines.
**	Generally this is because of a database error of some
**	sort.  Check the status number if no message was given.
**
** System Status:
**	Correct action according to status.
**
** User Action:
**	Correct and rerun.
*/


/*
** The following group of messages are specific to the DELOBJ Utility
*/

E_DE000A_Dobj_bad_flag
"Unrecognized flag %0c specified.\n
%1c\n
Execution ends."

E_DE000B_Dobj_no_file
"No file name for -include flag.\n
Execution ends.\n"

E_DE000C_Dobj_no_input
"No %0c matching the supplied name(s) and/or pattern(s) was found.\n
Execution ends."

E_DE000D_Dobj_open_fail
"Open failure processing -include file.\n
Execution ends."

E_DE000E_Dobj_read_fail
"Read failure processing -include file.\n
Execution ends."

E_DE000F_Dobj_nomatch
"No %0c matching the pattern %1c was found.\n"

E_DE0010_Dobj_notfound
"No %0c named %1c and owned by %2c was found.\n"

E_DE0011_Dobj_noall
"No objects owned by %0c were found.\n
Execution ends."

E_DE0012_Dobj_badall
"Incompatible parameter(s) specified with -all flag.\n
%0c\n
Execution ends."

E_DE0013_Dobj_notype
"No object type flag was specified.\n
%0c\n
Execution ends."

E_DE0014_Dobj_syntax
"The correct syntax is:\n
	DELOBJ dbname	[-report|-form|-joindef|-graph|-application|-qbfname]\n
			{objname} ...\n
			[-wildcard] [-silent] [-include 'file']\n
			[-uuser] [-Ggroupid]\n\n
			- or -\n\n
	DELOBJ dbname	-all\n
			[-silent]\n
			[-uuser] [-Ggroupid]\n\n
Note that any -Ggroupid parameter must be entirely enclosed\n
in double quotes."

E_DE0015_Dobj_multype
"Multiple object type flags were specified.\n
%0c\n
Execution ends."

