/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erim.msg -	Message file for class IM
**
** Description:
**	This file defines the messages for class IM.
**
** History:
**	1-sep-1987 (peter)
**		Created.
**	24-may-88 (bruceb)	Fix for bug 2668
**		Changed any Ingres' to INGRES; also changed INGRES/Menu
**		to INGRES/MENU.
**	04-oct-88 (sandyd)
**		Reorganized and changed wording of menu items and
**		explanations to implement some much needed user-interface
**		improvements requested by Training (and others).  Cleaned
**		up the names of the messages that were changed.
**	02-oct-89 (kathryn) Bug# 6242
**		E_IM0006 error message - Ingres logical name change.
**		Changed II_TEMP to II_TEMPORARY.
**	17-mar-1999 (mcgem01)
**		Replace references to the now obsolete I&O Guide.
**/

# define	_IM_CLASS	84

/*
**	imabf.qc
*/

E_IM0001_Your_virtual_machine_	
"Your virtual machine is not large enough to invoke ABF\n
To run ABF requires a larger virtual machine."
/*%
** Explanation:
**	In CMS only.
**
** System Status:
**	The request will be ignored.
**
** User Action:
**	Increase the size of the virtual machine, and run again.
*/

E_IM0002_INGRES_MENU_was_not_i	"ABF cannot be invoked from here.\n
Because INGRES/MENU was invoked from another process such as ABF,
you cannot startup ABF at this point.  To use ABF from INGRES/MENU,
you must have started up INGRES/MENU directly from the operating
system."
/*%
** System Status:
**	The request will be ignored.
**
** User Action:
**	Startup INGRES/MENU from the operating system.
*/

E_IM0004_Cannot_find_form "Cannot find form '%0c' in form index file.\n
The form '%0c' is not in the form index file.  Check with your
INGRES system manager to  see if there is an installation problem
with your message and form files."
/*%
** Explanation:
**	This is caused by a bad installation.  The file "rtiforms.fnx"
**	in the language files contains the forms.  The INGRES names
**	II_TEXT, II_LANGUAGE and II_FORMFILE can affect this.  See
**	the INGRES Getting Started Guide for details.
**
** System Status:
**	INGRES/MENU will terminate.
**
** User Action:
**	Check the installation.
*/

/*
**	immain.qc
*/

E_IM0005_Testing_could_not_beg	
"Testing could not begin\n
The testing system could not be started up.  Check to see if the
flags have been correctly specified."
/*%
** Explanation:
**	One of the testing options failed.  
**
** System Status:
**	INGRES/MENU will terminate.
**
** User Action:
**	Correctly specify the testing flags.
*/

E_IM0006_User_environment_impr
"User environment improperly set up.\n
One of the environment checks failed.  This usually means that
either: 1. the II_TEMPORARY INGRES name points to an area where you do
not have write access, or 2. II_TEMPORARY is not set, and you do not
have write access to your current working directory."
/*%
** Explanation:
**	When any INGRES frontend starts up, it first checks a few
**	things in the user environment to see if they are set up
**	correctly.  This is because disastrous problems could result
**	after much work has been done otherwise.  Currently, the only
**	check is to see if the temporary area is writable.  This is
**	either the user's current directory, or the area pointed to
**	by the INGRES name II_TEMPORARY.  If there would be a problem 
**	writing a file in this area, this error will occur.
**
** System Status:
**	The program will terminate.
**
** User Action:
**	Check the setting for II_TEMPORARY, or see if your working directory
**	is writable.  If not, move to another directory in which you
**	can write temporary files.
*/

E_IM0007_ingmenu__could_not_se	
"Could not set exception handler\n"
/*%
** Explanation:
**	This should only occur if an internal error occurs.
**
** System Status:
**	The program will terminate.
**
** User Action:
**	Contact your INGRES technical support representative.
*/

/*
**	imnaive.qc
*/

/*
** The following are the menu items and explanations given 
** for the various options in the top frame of the INGRES/MENU.
*/
F_IM000D_Tables		"Tables"
F_IM0002_Tables_expl "Create/examine tables or query/report on table data"

F_IM000E_Forms		 "Forms"
F_IM0003_Forms_expl "Create/edit/use forms for customized data access"

F_IM000F_JoinDefs	 "JoinDefs"
F_IM0004_JoinDefs_expl "Create/edit/use join definitions on multiple tables"

F_IM0010_Reports	 "Reports"
F_IM0005_Reports_expl	 "Create/edit/run reports"

F_IM0011_Graphs		 "Graphs"
F_IM0006_Graphs_expl	 "Create/edit/plot graphs"

/* capabilities allow ABF only */
F_IM0012_Applications	 "Applications"
F_IM0007_Applications_expl  "Create/edit/run 4GL applications"

/* capabilities allow "Vision" only */
F_IM0031_VisionOnly	 "Applications"
F_IM0032_VisionOnly_expl  "Create/edit/run Vision applications"

/* capabilities allow Vision and ABF */
F_IM0033_Vision_ABF	 "Applications"
F_IM0034_Vision_ABF_expl  "Create/edit/run Vision and ABF applications"

F_IM0013_Queries	 "Queries"
F_IM0019_Queries_expl "Query data using Query-By-Forms or a query language"
/*
** The following are reserved for tailored messages in environments 
** that may have fewer (or more) query options.
**
** F_IM001A_Queries_expl
** F_IM001B_Queries_expl
** F_IM001C_Queries_expl
** F_IM001D_Queries_expl
*/

/* The following is the explanation for the main frame help */
F_IM001E_Get_Help_on_Ingres_Me	 "Get Help on INGRES/MENU"

/* The following is the explanation for the main and submenu exits */
F_IM0020_Quit_Ingres_Menu	 "Quit INGRES/MENU"

/* The following is the title for help for main form of INGRES/MENU */
F_IM001F_INGRES_MENU	 "INGRES/MENU"

/*
** Menu items and explanations for the "Queries" submenu.
*/
F_IM0016_SQL		 "SQL"
F_IM000A_SQL_expl	 "Enter interactive SQL statements"

F_IM0017_QUEL		 "QUEL"
F_IM000B_QUEL_expl	 "Enter interactive QUEL statements"

F_IM0018_VQL		 "VQL"
F_IM000C_VQL_expl "Use the Visual-Query-Language capability of PC-Link"

F_IM0015_QBF		 "QBF"
F_IM0009_QBF_expl "Use Query-By-Forms to add/delete/change/view data"

/* The following is for the Help menu item */
F_IM001C_Get_Help_on_Queries	 "Get Help on Query options"

/* The following is the explanation to return from Queries */
F_IM0022_Return_to_top_form	 "Return to top form"

/* The following is the title of help for Queries submenu */
F_IM0023_INGRES_MENU___QUERIES	 "INGRES/MENU - Queries"

/*
** Explanation of the "Go" menuitem. NO LONGER USED (2/90 pete).
*/
F_IM0024_Go_expl	 "Invoke the selected option"

/*
** Null message for dynamically suppressing menuitems in imnaive.qsc.
*/
F_IM0025_Null	""

/*
** Menuitems for "Shell". Fix for jupbug 4641.
*/
F_IM0026_Shell	"Shell"
F_IM0027_CMS_Subset "CMS/Subset"
F_IM0028_Shell_Explanation	"Escape to the operating system"

/*
** Message statements so user won't be blank screened when starting subsystems.
*/
F_IM0029_Starting_ABF	"Starting ABF . . ."
F_IM0035_Starting_Vision	"Starting Vision . . ."
F_IM002A_Starting_QBF	"Starting QBF . . ."
F_IM002B_Starting_RBF	"Starting RBF . . ."
F_IM002C_Starting_Tblutil "Starting Tables Utility . . ."
F_IM002D_Starting_VIFRED  "Starting VIFRED . . ."
F_IM002E_Starting_VIGRAPH  "Starting VIGRAPH . . ."
F_IM002F_Starting_IQUEL  "Starting IQUEL . . ."
F_IM0030_Starting_ISQL  "Starting ISQL . . ."

/*
**	imreport.qc
*/

E_IM0008_Must_specify_a_report
"Must specify a report or table name\n
You must specify a name for an existing report or table in your
database."
/*%
** Explanation:
**	In response to a prompt, you must give a name of a report
**	or table from which to create a default report.
**
** System Status:
**	No report will be run.
**
** User Action:
**	Specify a name at the prompt.
*/

E_IM0009___s__is_a_bad_name
"'%0c' is a bad name for a report or table.\n
The name '%0c' is not a valid table or report name.  Valid names
begin with a letter or underscore character, and are followed
by letters, digits, the underscore character and a few special
characters."
/*%
** Explanation:
**	A check of the characters in the name came up with this
**	error.  See the INGRES reference manual section on names
**	for a description of valid names.
**
** System Status:
**	No report will be run.
**
** User Action:
**	Specify a valid name.
*/

S_IM000A_Output_File__Default_
"Output File [Default is set by report]: "
S_IM000B_Output_File__Default_
"Output File [Default goes to the Terminal]: "
		/*
		** These two messages are used to prompt a user for
		** the file name for the report output.
		*/

S_IM000C_Starting_REPORT_WRITE	"Starting REPORT WRITER..."
		/* This is a status message */
/*
**	imspawn.qc
*/

S_IM000D_Press_RETURN_to_clear
"Press RETURN to clear screen"
		/* This is a prompt for ascii terminals */

S_IM000E_Press_ENTER_to_clear_
"Press ENTER to clear screen"
		/* This is a prompt for 3270 terminals */

E_IM000F_Sorry____s_is_not_ava
"Could not startup up '%0c'.\n
The program '%0c' could not be started for some reason.  This can
be caused by a bad installation, not enough sub-process quota,
or redefined names of the program.  Check the environment."
/*%
** Explanation:
**	When trying to start a subprocess for the program mentioned,
**	some kind of error occurred.  This is usually because of 
**	a bad installation, or a sub process quota problem.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Check the installation and process quota.
*/

E_IM0010_Sorry___Could_not_sta
"Could not start '%0c' (status=%1d).\n
The program '%0c' could not be started for reason of status '%1d'.
This can be caused by a bad installation, not enough sub-process quota,
or redefined names of the program.  Check the environment."

/*
**	imutil.qc
*/

E_IM0014_General_Error
"General forms error occurred.\n
A general forms system error occurred.  Text of error is '%0c'."
/*%
** System Status:
**	Request is ignored.
**
** User Action:
**	Restart INGRES/MENU.
*/

/*%
** System Status:
**	subsystem is not called.
** User Action:
**	contact STC and report this error and surrounding conditions.
*/
E_IM0025_Missing_menuitem "Internal coding error.\n
An internal error occurred trying to locate and execute menuitem '%0c'.
The return code from routine '%2c' was %1d.
Note which menuitems you selected before this error occurred. Then write
down this error and notify your Site Technical Contact."
