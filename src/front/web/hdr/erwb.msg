/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	erwb.msg -	Message file for class WB
**
** Description:
**	This file defines the messages for class WB.
**	
**	GENERR - Self-explanatory message. This can be either an error or
**		 success message.
**
** History:
**	07-dec-95 (harpa06)
**		Created.
**	14-dec-95 (harpa06)
**		Added a more detailed explanation of how to install Ingres Web.
**	03-dec-95 (harpa06)
**		Changed the carriage return (\r) to a newline (\n) in
**		E_WB0002_CONTENT_TYPE since a carriage return is really not
**		needed but two newlines are.
**	21-feb-96 (harpa06)
**		Changed references to Ingres Web to Ingres/ICE.
**
**		Changed references to ingwebdb to icedb.
**
**		Added: E_WB0033_INST_CPY_GIF, E_WB0034_INST_CPY_JPG,
**		E_WB0035_INST_PROCESSING, E_WB0036_INST_PHASE1,
**		E_WB0037_INST_PHASE2, E_WB0039_INST_CGI_REFERENCE,
**		E_WB0038_INST_PHASE3, E_WB003A_INST_DIR_NOEXIST	
**		E_WB003B_BAD_DIR_CREATE, E_WB003C_INST_CPY_RWF)
**
**	14-mar-96 (harpa06)
**		Changed E_WB0024_INST_INTRO to not inform the user that 
**		permission is needed to write to the Ingres FILES directory
**		since the install program no longer writes to that directory.
**	26-apr-96 (harpa06)
**		Changed the E_WB0023_INST_ERROR message to be more meaningful.
**	30-apr-96 (harpa06)
**		Web server API/ADI project:
**
**		Added E_WB003D_RGEN_ICE_CMT for pre-existing Report-Writer
**		reports being sent to a Web client.
**
**		Changed log error messages to be more informative:
**
**		E_WB001C_NO_MATCH, E_WB000D_BAD_PROCVAR_COUNT, 
**		E_WB000F_BAD_PROCVAR_LENGTH, E_WB0010_BAD_PROCVAR_DATA,
**		E_WB000E_BAD_PROCVAR_TYPE, E_WB0012_PROCEXEC_FAIL,
**		E_WB0013_PROCEXEC_OK, E_WB001F_DEFAULT_ERROR,
**		E_WB0017_BAD_RW_EXECUTION. Added E_WB003E_ILLEGAL_STMT.
**		Changed the user action for E_WB0009_CONFIG_READ_ERROR.
**
**		Moved part of E_WB0024_INST_INTRO into E_WB0025_INST_SYNTAX
**		because there is no need to tell the user that knows how to
**		install internet communication that he must fulfill certain
**		requirements again. Also changed the syntax for installing
**		WWW communication with Ingres to include Spyglass ADI 
**		installations. Added install/uninstall messages for ICE
**		binaries and tutorial.
**
**		Changed explanation for E_WB0007_BAD_INSTALLATION.
**		Added E_WB003F_INST_BAD_OPTION, E_WB0040_INST_RM_ICE,
**		E_WB0041_INST_ICE, E_WB0042_INST_RM_TUTORIAL,
**		E_WB0043_INST_BAD_UNINST_TUTORIAL, E_WB0044_INST_UNINST_COMP,
**		E_WB0045_INST_COMPLETE, E_WB0046_INST_SGC_CFG_FIL_CMT,
**		E_WB0047_INST_MOD_CONFIG, E_WB0048_INST_CONF_FILE_OK  
**
**		Changed wording of E_WB0024_INST_INTRO and E_WB0025_INST_SYNTAX
**		to reflect the new uninstall feature.
**		Added E_WB0049_INST_BAD_SYNTAX.
**		Added E_WB004A_BAD_VERSION.
**		Changed E_WB0032_INST_CLOSING. 
**		Changed E_WB0003_BAD_CLIENT_DATA.
**		Changed E_WB0007_BAD_INSTALLATION.
**		Changed E_WB0004_OUT_OF_MEMORY.
**		Changed E_WB0011_BAD_DATABASE.
**		Changed E_WB003A_INST_DIR_NOEXIST to include non-existent files.
**		Changed E_WB0008_CONFIG_OPEN_ERROR to reflect Web server config
**		file read/write access.  
**		Standardized E_WB0002_CONTENT_TYPE.
**	26-may-96 (harpa06)
**		Bug #76664 - Change message E_WB001A_INTERNAL_ERROR to be more
**		meaningful.
**	01-Aug-96 (harpa06)
**		Added E_WB004B_NO_PASSWORD.
**	06-aug-96 (harpa06)
**		Changed E_WB0032_INST_CLOSING to not include the user running
**		the Web server having special permissions set up. This is no
**		longer needed since internet security has been fixed.
**	09-aug-96 (harpa06)
**		Added E_WB004C_INGRES_SYS_PASSWD.
**	14-aug-96 (harpa06)
**		Fixed typo in E_WB0043_INST_BAD_UNINST_TUTORIAL. (Bug #78282)
**	22-aug-96 (harpa06)
**		Fixed a few error messages that were grammatically incorrect.
**      11-oct-96 (natjo01)
**              Added "-remote" entry in iceinst syntax display.
**	26-dec-96 (harpa06)
**		Added E_WB004C_INGRES_SYS_PASSWD through
**		E_WB009F_INST_SHUTDWN_COMP2. Also modified other messages
**		to work with new HTML version of the ICE install/configure
**		program.
**      02-dec-1996 (wilan06)
**              Added E_WB004D_BAD_PROVAR_NAME, removed all ! from messages.
**      17-dec-1996 (wilan06)
**              Updated various messages for clarity
**      26-dec-1996 (harpa06)
**              Added E_WB004C_INGRES_SYS_PASSWD through
**              E_WB009F_INST_SHUTDWN_COMP and E_WB0100_INST_HTML_CLOSING
**              through E_WB0104_INST_OUTPUTD. Also modified other
**              messages to work with new HTML version of the ICE
**              install/configure program.
**	21-feb-1997 (harpa06)
**		Added E_WB0105_INST_BAD_CGI_DIR through 
**		E_WB012C_FORBIDDEN_ACTION.
**	03-mar-97 (harpa06)
**		Modified and added serveral messages.
**      07-mar-97 (harpa06)
**              Yanked E_WB0123_INST_LOG_ERRORF, E_WB0124_INST_LOG_INVALID and 
**		modified others.
**      18-mar-97 (harpa06)
**              Reformatted a few error messages so they can be viewed correctly
**              in the client's browser and in the administrator's log viewing
**              screen.
**      27-mar-97 (harpa06)
**		Modified E_WB0100_INST_HTML_CLOSING to include a link on a Web
**		page.
**
**		Modified E_WB0129_INST_SECU_EAPP_ACCEPT so that it reads to
**		return 0 for successful applications and a number greater than
**		one for FAILED applications.
**
**              Added E_WB0130_AGEN_ICE_CMT through E_WB013E_INST_DB_INVALID
**		Modified several other messages.
**	08-apr-97 (mcgem01)
**		Product has been renamed OpenIngres.
**      18-apr-97 (harpa06)
**              Corrected E_WB0049_INST_BAD_SYNTAX.
**		Added E_WB0140_BLBST_HASH_MARK and E_WB0141_BLBST_USAGE.
**      21-apr-97 (harpa06)
**		Modified E_WB0078_INST_OUTPUTD_LOCATION to indicate that it
**		must already exist.
**      01-may-97 (harpa06)
**              Added E_WB0142_INST_CPY_BLOB.
**      08-may-97 (harpa06)
**              Modified E_WB0046_INST_SGC_CFG_FIL_CMT,
**              E_WB0073_INST_END_CFG_COMMENT, E_WB0060_INST_NSES_ENTRY1
**              Added E_WB0143_INST_NSES_ENTRY4. 
**      13-may-97 (harpa06)
**              Reworded E_WB0094_INST_INSTALL_TUTDREXP to emphasize that the
**              database name is for the dynamic SQL tutorial. bug #82208.
**
**              Modified E_WB0135_INST_INSTALL_MCROEXP.
**              Modified E_WB0072_INST_DB_NAME_EXPLN.
**              Added E_WB0144_INST_NEW_SETTINGS. 
**
**              bug #82220: fixed misspelling of "dafault" to "default".
**      16-may-97 (harpa06)
**              Modified E_WB0101_INST_CONV_RESOURCE.
**      28-may-97 (harpa06)
**              Modified explanation and action for E_WB0011_BAD_DATABASE.
**      28-may-97 (harpa06)
**              Modified user action for E_WB0014_BAD_STATEMENT
**
**              Added E_WB0145_INST_SECU_DBOVR_ACCEPT through
**              E_WB0148_INST_MISCOP_DBOVR.
**      04-jun-97 (harpa06)
**              Fixed a few typos.      
**              Modified E_WB013B_INST_TUT_PASSWD.
**              Changed all references to "macroprocessor" and "Macroprocessor"
**              to "macro processor" and "Macro Processor".
**      11-jun-97 (harpa06)
**              Changed E_WB001E_DEFAULT_SUCCESS to sound grammatically
**              correct.
**      04-sep-97 (harpa06)
**              b84705 - Changed the "User Action" for message 
**              E_WB003E_ILLEGAL_STMT to correctly inform the user to use
**              the HTML configuration utility to enable execution of 
**              statements from an HTML page.
**      12-dec-97 (harpa06)
**              Modified E_WB004B_NO_PASSWORD
**      12-jan-98 (harpa06)
**              Fixed serveral messages with spelling and grammatical errors.
**      20-jan-98 (harpa06)
**              Added E_WB0149_ROWS_AFFECTED
**              Modified E_WB0011_BAD_DATABASE, E_WB0015_OK_STATEMENT,
**              E_WB0014_BAD_STATEMENT
**	10-apr-98 (mcgem01)
**		Product name change from OpenIngres to Ingres.
**      13-Aug-98 (fanra01)
**              Add E_WB014A_MISSING_II_SYSTEM.  This message is only for a
**              place holder since the application is not able to get this
**              message.
**      27-Aug-98 (fanra01)
**              Add a <PLAINTEXT> tag around output of query statements so that
**              any HTML tags embedded within the statement are not parsed.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
*/

#define	_WB_CLASS	225
	
/*
**	wbmain.c - Main module.
*/
E_WB0001_NO_CMD_LINE
"\nE_WB0001_NO_CMD_LINE - Program execution error.\n
Ingres/ICE must be executed from a running Web server.\n"

/*%
** Explanation:
**	Ingres/ICE only execute a request from a running Web server. 
**      Any other means of trying to use Ingres/ICE will fail.
**
** System Status:
**	Ingres/ICE will terminate.
**
** User Action:
**	Subhmit your Ingres/ICE request from a running Web server.
*/

/* HEADER */ E_WB0002_CONTENT_TYPE 	"Content-Type: text/html\n\n"

E_WB0003_BAD_CLIENT_DATA
"E_WB0003_BAD_CLIENT_DATA - Error reading client data."

/*%
** Explanation:
**	An error has occured when trying to read the client data using the
**	method defined on an HTML page.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Check to make sure that you are using a valid METHOD in your HTML
**	file in the FORM directive.
*/

E_WB0004_OUT_OF_MEMORY
"E_WB0004_OUT_OF_MEMORY - Out of memory."

/*%
** Explanation:
**	The system has run out of memory.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Check to make sure that you have enough memory available on your system
**	to process a Web client's request.
*/

E_WB0005_GET_OR_POST
"E_WB0005_GET_OR_POST - You must use either the GET or POST method."

/*%
** Explanation:
**	An illegal METHOD was used in the FORM directive in HTML file.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Check the HTML file and make sure that either the GET or POST method is
**	used to process data.
*/

E_WB0006_NO_II_SYSTEM
"E_WB0006_NO_II_SYSTEM - II_SYSTEM has not been defined by HTML developer."

/*%
** Explanation:
**	Ingres/ICE does not know the location of your Ingres 
**      installation.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Make sure that a valid value for "ii_system" is defined on your HTML
**	form or your Web server is able to supply the location of your
**      Ingres installation to Ingres/ICE.
*/

E_WB0007_BAD_INSTALLATION
"E_WB0007_BAD_INSTALLATION - Invalid INGRES installation.\n
An invalid INGRES installation location has been specified.\n"

/*%
** Explanation:
**	The value for "ii_system" given by the Web server or the user is an 
**	invalid location for an Ingres installation.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Make sure that the value of "ii_system" given to the current request
**      valid Ingres installation location.
*/

E_WB0008_CONFIG_OPEN_ERROR
"E_WB0008_CONFIG_OPEN_ERROR - Could not open configuration file."

/*%
** Explanation:
**	Either your Web server or the Ingres/ICE configuration file 
**	could not be opened for read/write access.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Make sure that the configuration file exists and it can be opened for 
**	read/write access.
*/

E_WB0009_CONFIG_READ_ERROR
"E_WB0009_CONFIG_READ_ERROR - Error reading configuration file."

/*%
** Explanation:
**	An error has occured while trying to read a configuration file. This
**	is caused by any of the following:
**
**		1) An entry missing in the Ingres configuration file 
**		   Ingres/ICE is looking for.
**
**              2) Your Ingres/ICE configuration file is corrupt.
**
**		3) One of the values for an entry in the Ingres 
**		   configuration file is invalid.
**
**		4) If the error occured while trying to read your Web server's 
**		   configuration file(s), it is possible your configuration 
**		   file(s) may be corrupt.
**
** System Status:
**	All processing stops. For Web server configuration files, no 
**	modifications are made.
**
** User Action:
**	Make sure your Ingres/ICE configuration is correct by reexecuting
**      the installation program to view your configuration values.  Also make 
**      sure your Web server's configuration file(s) are readable and not 
**      corrupt.
*/

/* GENERR */ E_WB000A_NO_USERID
/* GENERR */ "E_WB000A_NO_USERID - A user identification must be supplied."

/* GENERR */ E_WB000B_NO_DATABASE
/* GENERR */"E_WB000B_NO_DATABASE - A valid database name must be supplied."

E_WB000C_IM_LOST
"E_WB000C_IM_LOST - There is nothing for Ingres/ICE to do."

/*%
** Explanation:
**	Ingres/ICE has not been issued a request of which it can recognize.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Check your HTML form to make sure a request for Ingres/ICE has
**	been submitted to the Web server.
*/


/*
**	wbexqry.sc - EXECUTE PROCEDURE.
*/
E_WB000D_BAD_PROCVAR_COUNT
"E_WB000D_BAD_PROCVAR_COUNT - Invalid value for 'ii_procvar_count' for
<BR>database procedure '%s'. Value submitted: %s."

/*%
** Explanation:
**	An invalid value for the number of database procedure variables has 
**	been defined in "ii_procvar_count."
**
** System Status:
**	All processing stops.
**
** User Action:
**	Make sure there is a value assigned to "ii_procvar_count" and that it 
**      is valid. Check the documentation on valid values for ii_procvar_count.
*/

E_WB000E_BAD_PROCVAR_TYPE
"E_WB000E_BAD_PROCVAR_TYPE - Invalid datatype specified for
<BR>'ii_procvar_type%s' for database procedure '%s'. Value submitted: %s."

/*%
** Explanation:
**	A datatype is either missing or invalid for one of the database 
**	procedure variables.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Check the HTML form. Be sure that each database procedure variable has
**	a valid corresponding datatype.
*/

E_WB000F_BAD_PROCVAR_LENGTH
"E_WB000F_BAD_PROCVAR_LENGTH - Invalid value specified for
<BR>'ii_procvar_length%s' for database procedure '%s'. Value submitted: %s."

/*%
** Explanation:
**	An invalid value was specified for the length of a database
**	procedure variable name.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Correct the value for the database variable length in question.
*/

E_WB0010_BAD_PROCVAR_DATA
"E_WB0010_BAD_PROCVAR_DATA - Invalid data specified for
<BR>'ii_procvar_data%s' for database procedure '%s'. Value submitted: %s."

/*%
** Explanation:
**	The data provided is not valid for the assigned datatype for the 
**	database procedure variable.
**
** System Status:
**	All processing stops.
**
** User Action:
**	It is possible that the client entered the wrong type of data. In this
**	case, the client must enter data that is valid for the given datatype.
**
**	Is is also possibe that the value for 'ii_procvar_data' on the HTML
**	form is invalid. In this case, correct the data on the form.
*/

E_WB0011_BAD_DATABASE
"E_WB0011_BAD_DATABASE - Cannot establish a connection to database '%s'."

/*%
**
** Explanation:
**	A connection could not be made to the specified database.
**	
** System Status:
**	All processing stops.
**
** User Action:
**	Check the following:
**		1) The specified database exists in the given installation
**
**		2) The user identification and password specified has 
**                 permission to connect to the database.
*/

E_WB0012_PROCEXEC_FAIL
"E_WB0012_PROCEXEC_FAIL - Execution of database procedure '%s' has failed.
<BR>Values submitted: %s."

/*%
** Explanation:
**	The execution of the database procedure specified has failed to 
**	complete due to an internal error in the database procedure itself.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Given the data to be passed to the database procedure, check the
**	procedure to find out why execution has failed. Also, check the 
**      Ingres error log for any additional error messages.
*/

E_WB0013_PROCEXEC_OK
"E_WB0013_PROCEXEC_OK - Execution of procedure '%s' has completed
<BR>successfully. Values submitted: %s."

E_WB0014_BAD_STATEMENT
"E_WB0014_BAD_STATEMENT - An invalid query was submitted to be processed."

/*%
** Explanation:
**	An illegal dynamic SQL statement was asked to be executed.
**
** System Status:
**	All processing stops.
**
** User Action:
**  Check the following:
**      1) If you are using the macro processor and specified to override the
**         macro processor database in your SQL macro, check to see if this
**         option is enabled.
**
**      2) The Ingres error log for a possible reason why this error 
**         occured.
**
**      3) The dynamic SQL statement submitted is a valid statement.
*/

E_WB0015_OK_STATEMENT
"E_WB0015_OK_STATEMENT - The query statement:<P><PLAINTEXT>%s</PLAINTEXT><P>executed successfully."

/*
**	wbreport.sc - Report-Writer execution.
*/
/* GENERR */ E_WB0016_NO_TEMP_DIR
/* GENERR */ "E_WB0016_NO_TEMP_DIR - The II_TEMPORARY location does not exist."

E_WB0017_BAD_RW_EXECUTION
"E_WB0017_BAD_RW_EXECUTION - Unable to execute Report-Writer.
<BR>database=%s, userid=%s, report=%s, parameters=%s, tempfile=%s"

/*%
** Explanation:
**	Report-Writer was unable to execute the report given the parameters
**	passed to it.
**
** System Status:
**	All processing stops.
**
** User Action:
**	Perform the following:
**		1) Enable the log file and have the client resubmit their data.
**
**		2) After the client has received an error message, check the 
**		   log file to see what parameters were passed to 
**                 Report-Writer.
**
**		3) Using those parameters, execute Report-Writer on the 
**		   command line and view the results of the execution.
*/

E_WB0018_BAD_RW_EXECUTION
"E_WB0018_BAD_RW_EXECUTION - Unable to execute Report-Writer.
<BR>database=%s, userid=%s, report=%s, tempfile=%s"

/*%
** Explanation:
**	See the explanation for E_WB0017_BAD_RW_EXECUTION.
*/

E_WB0019_BAD_RW_EXECUTION
"E_WB0019_BAD_RW_EXECUTION - Unable to execute Report-Writer.
<BR>No memory available to print error message."

/*%
** Explanation:
**	There was no memory available to print out the parameters that were 
**	passed to Report-Writer. Close a few applications and try executing the
**	report again. Also, see the explanation for E_WB0017_BAD_RW_EXECTUTION
**	as to why Report-Writer failed to execute the report.
*/

E_WB001A_INTERNAL_ERROR
"E_WB001A_INTERNAL_ERROR - An internal error has occurred. \n
Please create a log file and rerun the request. If you cannot resolve the\n
problem using the information in your log file, please contact your\n
Ingres technical support representative."

/*%
** Explanation:
**	An internal error has occured within Ingres/ICE.
**	
** System Status:
**	All processing stops.
**
** User Action:
**	Create a log file for the request and rerun it. Using the output in
**	the log file, you should be able to resolve the problem. If you cannot,
**	contact your Ingres technical support representative.
*/

E_WB001B_NO_MATCH
"E_WB001B_NO_MATCH - No data is available given the query submitted."

/*%
** Explanation:
**	Given the parameters sent to Report-Writer, there were no matching
**	rows in the database to display to the client.
**
** System Status:
**	Report processing is complete.
**
** User Action:
**	If this is an error, using the client parameters, execute Report-Writer
**	from the command line to validate the error and contact Ingres
**	technical support.
*/	

E_WB001C_NO_MATCH
"E_WB001C_NO_MATCH - No data available for report %s given the data submitted
<BR>{database=%s, userid=%s, and parameters=%s}."

/*%
** Explanation:
**	See the explanation for E_WB001B_NO_MATCH.
*/

E_WB001D_RW_COMPLETE
"E_WB001D_RW_COMPLETE - Execution of report '%s' has completed successfully
<BR>given the parameters {%s}"

/*
**	wbmesg.c - Message displays.
*/
/* GENERR */ E_WB001E_DEFAULT_SUCCESS 
/* GENERR */ "Your request has completed successfully."

E_WB001F_DEFAULT_ERROR
"E_WB001F_DEFAULT_ERROR - An error has occured. Please contact the
<BR>server administrator."

/*%
** Explanation:
**	This is the message that is sent to the client when a message or a URL
**	has not been defined on an HTML form for Ingres/ICE to process.
**
** System Status:
**	Ingres/ICE displays this message to the client.
**
** User Action:
**	To minimize the confusion of the client, it is recommended to always 
**	define a valid value for either "ii_error_url" or "ii_error_message."
**	To determine what might have gone wrong with the transaction, use
**	the request monitoring facility in the installation/configuration
**      program to view the results of your Web client's requests.
*/

/* GENERR */ E_WB0020_DEFAULT_HTML_TITLE	"Form Submission Results"

/* GENERR */ E_WB0021_DEFAULT_HTML_GHEADER	"Submission Complete."

/* GENERR */ E_WB0022_DEFAULT_HTML_BHEADER	"An error has been detected."


/*
**	wbinstall.c - Installation program
*/
E_WB0023_INST_ERROR
"\nE_WB0023_INST_ERROR - Install/uninstall error.\n
An error has occured during the install/uninstall process.\n\n
Have you configured Ingres/ICE using the Configuration-By-Forms\n
utility?\n
Are you using the Ingres userid during the installation process?\n
Does the Ingres userid have write access to your root HTML and CGI directories?\n
\n Please be sure that the above conditions are met and rerun the utility\n
program.\n"

/*%
** Explanation:
**	An error has occured during the install/uninstall process.
**	This can be caused by any one or more of the following:
**	
**		1) Ingres/ICE not been configured using the 
**		   Configuration-By-Forms utility
**
**		2) The Ingres user id is not being used during the installation
**		   process.
**
**		3) The Ingres userid does not have write access to your root
**		   HTML and CGI directories.
**
**		4) The tutorial could not be created.
**
**		5) The tutorial database could not be destroyed.
**
** System Status:
**	The installation is terminated.
**
** User Action:
**	Make sure that the above conditions are met and rerun the installation
**	utility.
*/

E_WB0024_INST_INTRO
" This program will configure and install/uninstall Ingres/ICE\n
components for or from your Web server.\n\n"
 
E_WB0025_INST_SYNTAX
" To use this utility successfully, your Web server must be running under the\n
'ingres' user id.\n"

/* */ E_WB0026_INST_HTML_DIR
"<STRONG>Primary HTML Directory:</STRONG>\n<BR>\n
The primary HyperText Markup Language (HTML) document directory is the\n
<strong>root</strong> directory for your HTML documents. If you do not know what this is,\n
please consult your Web server's configuration.\n<P>\n
Enter the full path of your Web server's primary HTML document directory.\n"

/* */ E_WB0027_INST_INVALID_DIR
/* */ "You have entered an invalid directory.\n\n"

/* */ E_WB0028_INST_CGI_DIR
/* */ "Enter the name of your Common Gateway Interface (CGI) directory.\n\n"

E_WB0029_INST_WEB_SERVER
"<STRONG>Web Server Name:</STRONG>\n<BR>\n
Enter the name or IP address, including the port number of your Web server\n
(e.g. - webserver.yourcompany.com:80):\n"

/* */ E_WB002A_INST_START_SETUP
/* */ "Starting the Ingres/ICE tutorial setup . . .\n\n"

/* */ E_WB002B_INST_POPULATE
/* */ "Populating the Ingres/ICE tutorial database '%s' . . .\n"

E_WB002C_INST_CPY_EX1
"\nCopying Ingres Web Example #1 to the 'icedb' database . . .\n\n"

E_WB002D_INST_CPY_EX2
"\nCopying Ingres Web Example #2 to the 'icedb' database . . .\n\n"

/* */ E_WB002E_INST_CPY_HTML2ROOT
/* */ "\n\nCopying Ingres Web demo files . . .\n\n" 

/* */ E_WB002F_INST_CPY_EXE2CGIBIN
/* */ "\nCopying the Ingres/ICE executable . . .\n\n"

/* */ E_WB0030_INST_CREATE_CONFIG
/* */ "\nCreating the Ingres/ICE configuration file:\n
'%s' . . .\n\n\n"

E_WB0031_INST_BAD_CONFIG_CREAT
"E_WB0031_INST_BAD_CONFIG_CREATE - Error creating configuration file.\n\n"

/*%
** Explanation:
**	An error has occured during the creation of the configuration file.
**	This can be caused by not having permission to write to the directory
**	where the configuration file is being created.
**
** System Status:
**	The installation is terminated.
**
** User Action:
**	Be sure that you are user "ingres" when running the installation 
**	program and that the directory where the configuration file is being
**	created is writeable.
*/

E_WB0032_INST_CLOSING
"\nPlease be sure to create the user id that executes programs in your CGI\n
directory as an INGRES user before using Ingres/ICE.\n\n\n
If your Web server is currently running, you may now point your Web browser to\n
the following URL:\n\n
     http://%s/ca-ice/iiinvidx.html\n\n"

/* */ E_WB0033_INST_CPY_GIF 	"Copying Compuserve GIF file(s) . . .\n"

/* */ E_WB0034_INST_CPY_JPG 	"Copying JPG file(s) . . .\n"

/* */ E_WB0035_INST_PROCESSING 	"Processing HTML files . . .\n"

/* */ E_WB0036_INST_PHASE1 	"Phase one . . .\n"

/* */ E_WB0037_INST_PHASE2 	"Phase two . . .\n"

/* */ E_WB0038_INST_PHASE3 	"Phase three . . .\n"

E_WB0039_INST_CGI_REFERENCE
"Enter how you would reference your CGI-BIN directory through a URL:\n\n"

E_WB003A_INST_DIR_NOEXIST
"E_WB003A_INST_DIR_NOEXIST - A mandatory directory or file does not exist.\n"

/*%
** Explanation:
**	One of the directories that have been configured for the 
**	Ingres/ICE installation or for Ingres/ICE itself does not 
**	exist; or the installation program cannot find your Web servers'
**	configuration file; or Ingres/ICE does not have access to a 
**      directory you have specified.
**
** System Status:
**	If the installation is running, it is terminated.
**	If Ingres/ICE is running, processing is terminated.
**
** User Action:
**	Make sure that the directories and files specified for internet
**	communication in CBF exist.
*/

E_WB003B_BAD_DIR_CREATE
"E_WB003B_BAD_DIR_CREATE - Unable to create tutorial directory '%s.'\n" 

/*%
** Explanation:
**	An error has occured when creating the Ingres/ICE tutorial
**	directory. It is possible that the installation program does not have
**	permission to create this directory.
**
** System Status:
**	The installation is terminated.
**
** User Action:
**	Make sure that you are user "ingres" when running the installation 
**	program and that the ingres userid has permission to create 
**      subdirectories within the directory where you keep your HTML documents.
*/

/* */ E_WB003C_INST_CPY_RWF 	
"Copying Report-Writer report specification files . . .\n"

/*
**	Pre-exising report comment message
**	     RGEN - Report GENerated by
*/

/* */ E_WB003D_RGEN_ICE_CMT		
"  Generated by Ingres/ICE\n Report Name: %s\n"


E_WB003E_ILLEGAL_STMT
"E_WB003E_ILLEGAL_STMT - An attempt to execute an SQL statement from an HTML
<BR>page was made when this option is disabled. Statement attempted: <PLAINTEXT>%s</PLAINTEXT>."

/*%
** Explanation:
**	A Web client has tried to execute an SQL statement from a Web page
**	using the "ii_query_statement" variable when this feature is disabled.
**
** System Status:
**	All processing stops.
**
** User Action:
**	If you want to have Web clients execute SQL statements using the
**	"ii_query_statement" variable, make sure that this option is enabled
**	in the "Configure Security Options" section of the Ingres/ICE HTML
**	configuration program.
*/

E_WB003F_INST_BAD_OPTION
"\nE_WB003F_INST_BAD_OPTION - Invalid option: '%s'.\n\n"


/*%
** Explanation:
**	An invalid option was specified on the command line. 
**
** System Status:
**	The installation program terminates.
**
** User Action:
**	Rerun the installation program and specify the proper syntax in the 
**	correct order on the command line. To view the correct syntax of the
**	utility, enter the name of the utility on the command line.
*/

/* */ E_WB0040_INST_RM_ICE	
"\nUninstalling Ingres Internet Communication . . .\n\n"

/* */ E_WB0041_INST_ICE
"\nCopying Internet Communication files . . .\n\n"

/* */ E_WB0042_INST_RM_TUTORIAL "Uninstalling the %s tutorial . . .\n\n"

E_WB0043_INST_BAD_UNINST_TUTOR
"\nE_WB0043_INST_BAD_UNINST_TUTOR - Cannot uninstall the tutorial.\n
The tutorial could not be fully uninstalled.\n\n
Is the directory write-protected?\n
Is the tutorial database locked?\n
Has the tutorial already been uninstalled?\n\n
Please make sure that the above conditions have been met and rerun the\n
installation program again.\n"

/*%
** Explanation:
**	The uninstaller could not fully uninstall the Ingres/ICE
**	tutorial. 
**
** System Status:
**	The uninstall program quits.
**
** User Action:
**	Make sure that the following conditions have been met:
**		- The tutorial directory is writable by the "ingres" userid.
**		- The tutorial directory can be removed.
**		- All files in the tutorial directory can be removed.
**		- The tutorial database does not have a lock on it.
**              - The tutorial is not already uninstalled.
**
**	Once these conditions are met, rerun the uninstall program.
*/

/* */ E_WB0044_INST_UNINST_COMP	
"\nThe uninstall process has completed successfully.\n\n"

/* */ E_WB0045_INST_COMPLETE	
"The installation process has completed successfully.\n\n"

/* */ E_WB0046_INST_SGC_CFG_FIL_CMT
"%sAdded by Ingres/ICE. Do not modify this line or the next line.\n"

/* */ E_WB0047_INST_MOD_CONFIG	
"Modifying Web server configuration file: \"%s\" for Web server \"%s\".\n
A backup file with with an extension of \".bak\" will be made for you . . .\n\n"

E_WB0048_INST_CONF_FILE_OK
"E_WB0048_INST_CONF_FILE_OK - Configuration file already modified.\n
This Web server configuration file has already been modifed. No changes will\n
be made.\n\n"

E_WB0049_INST_BAD_SYNTAX
"\n Usage: iceinst [-d <CGI directory>] [-u <CGI URL>] [-s <Web server name>]\n
		-b <Web browser>\n\n
Where:\n
        CGI directory   = The full path to your Web server's CGI directory (if\n
                          not already configured)\n\n
        CGI URL         = The Uniform Resource Location (URL) to your CGI\n
                          directory (if not already configured)\n\n
        Web server name = The name of your Web server known on the internet\n
                          and/or the intranet (if not already configured)\n\n
        Web browser     = The full path and file name of your Web browser\n"

E_WB004A_BAD_VERSION
"E_WB004A_BAD_VERSION - Unsupported Web server.\n
The Ingres/ICE server does not support the Web server version you are\n
currently using. Internet communication with Ingres will be inactive\n
at this time."

/*%
** Explanation:
**	The Ingres/ICE server does not support the version of the Web
**	server you are currently using.
**
** System Status:
**	The Ingres internet communication server fails to load.
**
** User Action:
**	Refer to the documentation to see what versions of your server are
**	currently supported.
*/

E_WB004B_NO_PASSWORD
"E_WB004B_NO_PASSWORD - No password specified.<BR>A password has not been specified for the given request."

/*%
** Explanation:
**	A mandatory password for the userid given has not been specified.
**
** System Status:
**	The Ingres request is rejected.
**
** User Action:
**      Specify a password along with the previously submitted userid for the
**      request.
*/

/* */ E_WB004C_INGRES_SYS_PASSWD
" To use the tutorial, you must provide the system password to the\n
Ingres account on this machine.\n\n
Please enter the password to the Ingres system account below. To cancel, press\n
the ENTER key.\n\n"

/* */ E_WB004D_INST_HTML_TITLE "Ingres/ICE Configuration - "

/* */ E_WB004E_INST_WSERVER_TYPE
"<STRONG>Supported Web Server:</STRONG>\n<BR>\n
Select a Web server you have already installed that Ingres/ICE\n
supports. If you do not know the name of your Web server, please consult your\n
Web server's documentation before continuing. If you still do not know the
name of your Web server or your Web server is not in the list below, select\n
the default type <strong>%s</strong>.\n"

/* */ E_WB004F_INST_INSTALL_TYPE
"Select the operation you would like to perform:\n"

/* */ E_WB0050_INST_INSTALL_ICEBIN
"<STRONG>Ingres/ICE Binary:</STRONG>\n<BR>\n
The Ingres/ICE binary is the heart of Ingres/ICE. This is the\n
component that is responsible for processing all Ingres/ICE Web client\n
request.\n<P>\n"

/* */ E_WB0051_INST_ICE_CGI_DIR
"<STRONG>Primary CGI Directory:</STRONG>\n<BR>\n
Your primary Common Gateway Interface (CGI) directory is where the majority\n
of your programs executed by your Web clients are stored. Ingres/ICE\n
uses this directory to execute applications requested from an HTML form. If\n
you do not know what your primary CGI directory is, please consult your Web\n
server's configuration before continuing.\n<P>\n
Enter the full path to your primary Common Gateway Interface (CGI)\n
directory:\n"

/* */ E_WB0052_INST_DALLW_DSQL_QUERY
/* */ "Do not accept dynamic SQL queries from HTML Pages\n"

/* */ E_WB0053_INST_ALLOW_DSQL_QUERY
/* */ "Accept dynamic SQL queries from HTML Pages\n"

/* */ E_WB0054_INST_HTML_WELCOME
"This is the Ingres/ICE main configuration screen. If you are going to\n
install or uninstall any of the Ingres/ICE components, you must\n
have at <EM>least</EM> the following configured:\n<UL>\n
<LI>Web Server Information\n
<LI>At least one output directory\n
<LI>Install and/or Uninstall Options\n</UL>\n
Other options can be configured at your leisure.\n<P>\n
Once you have met the minimum requirements above, select <strong>Install\n
and/or\n Uninstall Selected Components</strong> to install and/or uninstall\n
the\n Ingres/ICE components you have chosen.\n<P>\n
When you are finished with the configuration program, select <strong>Close\n
Configuration Program</strong> to correctly shut down the\n
Ingres/ICE configuration program.\n" 

/* */ E_WB0055_INST_PICK_CFG  "Select one of the following options:\n"

/* */ E_WB0056_INST_DBDEF_TIMEOUT  "Database connection timeout value:\n"

/* */ E_WB0057_INST_SECONDS  " seconds"

/* */ E_WB0058_INST_DBDEF_DBNAME  "Default database name:\n"

/* */ E_WB0059_INST_DBDEF_USER  "Default user name:\n"

/* */ E_WB005A_INST_CGI_URL
"<STRONG>CGI Directory URL:</STRONG>\n<BR>\n
A Uniform Resource Location (URL) is used to retrieve data from a Web server\n
on the World Wide Web using a Web browser. For a detailed discussion of URL's,\n
please see <strong>NCSA's URL Guide</strong> located at\n
<strong>http://www.ncsa.uiuc.edu/demoweb/url-primer.html.</strong>\n<P>\n
The URL Ingres/ICE is looking for is the representation of how\n
your would specify your primary CGI directory (specified above) using a Web\n
browser. If you do not know what this is, please consult your Web server's\n
documentation before continuing.\n<P>\n
Enter the Uniform Resource Location (URL) to the CGI directory specified\n
above.\n"

E_WB005B_INST_BAD_BROWSER
"\nE_WB005B_INST_BAD_BROWSER - Bad Web browser:\n\n
\t\t%s\n\n"

/*%
** Explanation:
** The full path and filename to a Web browser has not been specified on the
** command line or the one you have specified does not exist.
**
** System Status:
**	The Ingres installation program quits.
**
** User Action:
**  Please make sure to specifiy the full path and executable name of your Web
**  browser on the command line. If you do not have a Web browser, you cannot
**  install, uninstall or configure Ingres/ICE.
*/

/* */ E_WB005C_INST_CONF_CGIDIR
"Configured Common Gateway Interface Directory '%s' . . .\n"

/* */ E_WB005D_INST_CONF_CGIURL
"Configured Common Gateway Interface Uniform Resource Location '%s' . . .\n"

/* */E_WB005E_INST_MAIN_SCREEN  "Main Configuration Screen"

/* */ E_WB005F_INST_TUT_WRNING
"Since you are installing a tutorial, on some machines, creating a database\n
takes a long time. During the database creation process, if your Web server\n
times out, perform the following:\n 
<OL>\n<LI>\nIncrease your Web server's CGI timeout value. A value of about 5\n
minutes should be sufficient. For instructions on configuring your CGI timeout\n
value, please see your Web server's CGI timeout configuration instructions.\n
<LI>Restart your Web server.\n
<LI>Click the \"reload\" button on your Web browser. The install should work\n
successfully.\n
<LI>If the process still fails, repeat the above steps while increasing your\n
CGI timeout value by 2 minutes.\n
<LI>Once the process is complete, reconfigure your CGI timeout value to it's\n
orignal value.\n</OL>\n<P>\n"

/* */ E_WB0060_INST_NSES_ENTRY1
"Init fn=\"load-modules\" shlib=\"%s\" funcs=\"%s\"" 

/* */ E_WB0061_INST_NSES_ENTRY2
"Service fn=\"%s\" method=\"(GET|POST)\" type=\"magnus-internal/ca-ice\""

/* */ E_WB0062_INST_NSES_ENTRY3 "type=magnus-internal/ca-ice exts=%s\n"

E_WB0063_INST_BAD_INPUT_DATA "E_WB0063_INST_BAD_INPUT_DATA - Invalid data.\n
You have submitted invalid information to the configuration program. Please\n
correct the problem and try again.\n"

/*%
** Explanation:
**  The data you have specified to the configuration program is invalid.
**
** System Status:
**  The installation program brings you back to the screen that caused the
**  error.
**
** User Action:
**  Please make sure that the data you are about to submit to the installation
**  program is correct before submitting it.
*/

E_WB0065_INST_NOTHINGTODO "E_WB0065_INST_NOTHINGTODO - Nothing to do.\n
There was nothing to install/uninstall\n"

/*%
** Explanation:
**  You have not specified any Ingres/ICE components to install or
**  uninstall.
**
** System Status:
**	The Ingres install/uninstall process quits.
**
** User Action:
**  Please make sure you have specified an Ingres/ICE component to install
**  or uninstall before selecting the option to install/uninstall selected
**  products.
*/

E_WB0066_INST_BAD_INSTALL 
"E_WB0066_INST_BAD_INSTALL - %d error(s) detected during the install/uninstall\n
process.\n"

/* */ E_WB0067_INST_DB_DEFAULTS "Database Defaults"

/* */ E_WB0068_INST_CONFIRM_TITLE "Confirm Install/Uninstall"

/* */ E_WB0069_INST_SETTING_CONFIRM
"Are you sure the above settings are the ones you would like to use? This will\n
be your last chance to change your mind before the install/uninstall process\n
begins.\n"

/* */ E_WB006A_INST_DB_USERID_EXPLN
"<STRONG>Default User Name:</STRONG>\n<BR>\n
The default user name is used if one has not been specified on an HTML form.\n
If a default user name is not defined, it must be specified on an HTML form\n
for every Ingres/ICE request. The default is user <strong>ingres</strong>.\n"

/* */ E_WB006B_INST_INSTOP_SCR
"This is the installation options configuration screen. Here, you can select\n
any of the Ingres/ICE components you would like to install or\n
uninstall.\n"

/* */ E_WB006C_INST_INST_OPTIONS "Installation Options"

/* */ E_WB006D_INST_OUTPUTD_NONE "No directories are configured\n"

/* */ E_WB006E_INST_INST_ICE "Install Ingres/ICE"

/* */ E_WB006F_INST_UNINST_ICE "Uninstall Ingres/ICE"

/* */ E_WB0070_INST_DB_DEFAULTS_SCR
"This is the database defaults configuration screen. Here, you can configure:\n
<UL>\n<LI>The database connection timeout value\n
<LI>The default database name\n
<LI>The default user name\n</UL>\n"

/* */ E_WB0071_INST_DB_CT_EXPLAIN
"<STRONG>Database Connection Timeout:</STRONG>\n<BR>\n
The database connection timeout value is used to determine when to disconnect\n
a user from a database session. If a user connected to a specified database\n
has not requested any database activity before the timeout value is reached,\n
the user is automatically disconnected and must reconnect before processing\n
continues. Enabling the connection timeout value allows clients to make many\n
requests to the same database without reconnecting first each time. This\n 
increases the processing speed of a user's request. The connection timeout\n
value is used when Ingres/ICE and a Web server are initialized\n
together. A value of \"0\" means\n to disconnect users from their database\n
sessions as soon as their request is complete. This has pretty much the same\n
effect as running Ingres/ICE as a CGI application. The default value\n
is <strong>300 seconds (5 minutes)</strong>\n"

/* */ E_WB0072_INST_DB_NAME_EXPLN
"<STRONG>Default Database Name:</STRONG>\n<BR>\n
The default database name, which may also contain a nodename, is used if one\n
is not specified on an HTML form. The default is <strong>icedb</strong>.\n"

/* */ E_WB0073_INST_END_CFG_COMMENT 
/* */ "----- End Ingres/ICE modification -----"

/* */ E_WB0074_INST_INST_TUT "Install the dynamic SQL tutorial"

/* */ E_WB0075_INST_UNINST_TUT "Uninstall the dynamic SQL tutorial"

/* */ E_WB0076_INST_OUTPUTD_SCRN
"This is the Output Directories configuration screen. Here, you can configure\n
as many output directories as you would like to store client output. Each\n
output directory contains a directory label, a directory location, and a file\n
expiration time. The user running your Web server must have read and write\n
permission to all configured locations.\n"

/* */ E_WB0077_INST_OUTPUTD_LABEL
"<STRONG>Directory Label:</STRONG>\n<BR>\n
The directory label is used to identify the location where client output is\n
stored. This label is defined in the Ingres/ICE reserved word\n
<strong>ii_output_dir</strong> on an HTML form. By using directory labels,\n
clients that view your HTML source cannot determine the physical location of\n
Ingres/ICE request output. If a client knew where your output was\n
stored, they may attempt to break into your Web server and try to access other\n
files in the same directory.\n"

/* */ E_WB0078_INST_OUTPUTD_LOCATION
"<STRONG>Directory Location:</STRONG>\n<BR>\n
A directory location is a subdirectory path underneath your primary HTML\n
directory where Ingres/ICE request output is stored. The Uniform\n
Resource Location (URL) mapping of this directory must be of the same name.\n
If this directory does not already exist, please create it now.\n"

/* */ E_WB0079_INST_OUTPUTD_EXP_TIME
"<STRONG>File Expiration Time:</STRONG>\n<BR>\n
The file expiration time is used by Ingres/ICE to remove any files in\n
a directory location that have been in existence longer than the configured\n
expiration value. This is how Ingres/ICE cleans up after itself.\n
The longer the expiration time, the longer clients have access to a particular\n
file. It is recommended that all file expiration times are <strong><em>at\n
least 30 seconds</em></strong> so your Web server has enough time to send\n
files in your output directories to your Web clients. If any expiration time\n
is less than the recommended value, it will be highlighted in\n
<strong><font color=\"red\">red</font></strong>.\n"

/* */ E_WB007A_INST_OUTPUTD_DIRLABEL "Directory label:\n"

/* */ E_WB007B_INST_OUTPUTD_DIRLOCAT 
/* */ "Directory location: <strong>%s%c</strong>\n"

/* */ E_WB007C_INST_OTUPUTD_FILEEXPT "File expiration time:\n"

/* */ E_WB007D_INST_OTUPUTD_LABEL2
"At least one output directory must be configured:\n<UL>\n
<LI>To add a directory location, enter all information below. If you would\n
like your new location become the default, select the <strong>Use as the\n
default location</strong> checkbox.\n
<LI>To modify the attributes of a configured location, enter the directory\n
label, the attributes you wish to modify, and select <strong>Add/Modify</strong>.\n
<LI>To remove a directory location, enter the directory label and select\n
<strong>Delete</strong>.\n</UL>\n
When you are finished configuring your directories, select <strong>Cancel</strong>\n."

/* */ E_WB007E_INST_SECU_OPTIONS_SCR
"This is the security options screen. Here, you can configure\n
Ingres/ICE options that may render your system inoperable if not\n
confgured carefully.\n"

/* */ E_WB007F_INST_SECU_OPTIONS "Security Options"

/* */ E_WB0080_INST_SECU_DSQL_ACCEPT
"<STRONG>Accept Dynamic SQL Queries from Web Pages:</STRONG>\n<BR>\n
Executing dynamic SQL queries from an HTML form is a powerful feature of\n
Ingres/ICE. It can be used to render a database or a Ingres\n
installation unusable if not used correctly. To have complete control over\n
what is happening to your corporate data on the internet, disable this option\n
to prevent Web clients from executing dynamic SQL queries of any kind from\n
HTML forms.\n" 

/* */ E_WB0081_INST_WSERVER_INFO "Web Server Information"

/* */ E_WB0082_INST_WSERVER_INFO_SCR
"This is the Web server information configuration screen. Here, you can\n
configure the bulk of the information Ingres/ICE needs to know about\n
your Web server. What Ingres/ICE needs to know here is:\n<UL>\n
<LI>The type of Web server you have\n
<LI>Your Web server's configuration directory\n
<LI>The name and port number of your Web server known on the internet\n
<LI>Your Web server's primary HTML directory\n
<LI>Your Web server's primary Common Gateway Interface (CGI) directory\n
<LI>The Resource Location (URL) mapping to your primary CGI directory\n</UL>\n"

/* */ E_WB0083_INST_WSERVER_CFG_DIR
"<STRONG>Web Server Configuration Directory:</STRONG>\n<BR>\n
Your Web server's configuration directory is the location where your critical\n
Web server information is stored. To have your Web server communicate\n
directly with Ingres, the Ingres/ICE configuration program needs\n
to know where this directory is located. If you do not know where your\n
configuration directory is located, please consult your Web server's\n
configuration before continuing.\n<P>\n
Enter the path to your Web server's configuration directory. This entry will\n
be ignored if <strong>%s</strong> is selected.\n<BR>\n"

/* */ E_WB0084_INST_INST_RESULTS "Install/Uninstall Results"

/* */ E_WB0085_INST_CFGOP_HDR_OUTDIR "Output Directories"

/* */ E_WB0086_INST_CFGOP_LABEL "Label:\n"

/* */ E_WB0087_INST_CFGOP_LOCATION "Location (subdirectory):\n"

/* */ E_WB0088_INST_CFGOP_FEXPTIME "File Expiration Time:\n"

/* */ E_WB0089_INST_DBDEF_CONNT "Connection Timeout:\n"

/* */ E_WB008A_INST_DBDEF_DBNAME "Database Name:\n"

/* */ E_WB008B_INST_DBDEF_USERNAME "User Name:\n"

/* */ E_WB008C_INST_MISCOP_DSQL "Accept Dynamic SQL Queries:\n"

/* */ E_WB008D_INST_WSOPT_CONF_WS "Configured Web Server:\n"

/* */ E_WB008E_INST_WSOPT_CONF_WSDIR "Configuration Directory:\n"

/* */ E_WB008F_INST_WSOPT_SERVERNAME "Server Name:\n"

/* */ E_WB0090_INST_WSOPT_MAINDOCDIR "Main Document Directory:\n"

/* */ E_WB0091_INST_WSOPT_CLNTAPPDIR 
"Common Gateway Interface (CGI) Directory:\n"

/* */ E_WB0092_INST_WSOPT_CLNTAPPURL 
"Uniform Resource Location (URL) to CGI directory:\n"

E_WB0093_INST_INSTALL_TUTDB "Tutorial database name:\n"

E_WB0094_INST_INSTALL_TUTDREXP
"<STRONG>Dynamic SQL Tutorial Database Name:</STRONG>\n<BR>\n
If you choose to install the Dynamic SQL tutorial, a database\n
will be used as the Dynamic SQL tutorial database. This database is\n
populated with the information required to run the tutorial. The database name\n
you enter here will be the database used for the tutorial. To create the\n
database in another installation, be sure to include it's nodename when\n
specifying the database.\n<P>\nThe default database name is <strong>icedb\n
</strong>.\n"

E_WB0095_INST_OUPUTD_NOEXP 
"E_WB0095_INST_OUTPUTD_NOEXP - Invalid expiration time.\n
You did not supply a file expiration time or the one you have supplied is not\n
valid. Please correct the problem and try again.\n"

/*%
** Explanation:
**  The file expiration time you have supplied is invalid or you forgot to 
**  supply a time.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that the file expiration time you are about to submit to
**  the installation program is correct before submitting it.
*/

E_WB0096_INST_OUTPUTD_BADDIR
"E_WB0096_INST_OUTPUTD_BADDIR - Invalid directory location.\n
You did not supply a directory location or the one you have supplied is not\n
valid. Please correct the problem and try again.\n"

/*%
** Explanation:
**  The directory location you have supplied is invalid or you forgot to 
**  supply a location.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that the directory location are about to submit to
**  the installation program is a valid subdirectory of your primary HTML
**  directory.
*/

E_WB0097_INST_OUTPUTD_NODLBL
"E_WB0097_INST_OUTPUTD_NODLBL - No directoy label specified.\n
Either you did not supply a directory label or the one you have specified does\n
not exist. Please correct the problem and try again.\n"

/*%
** Explanation:
**  You forgot to supply a directory label for the output directory you wish to
**  use with Ingres/ICE or the one you have specified does not exist.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that you supply a valid directory label.
*/

E_WB0098_INST_DBDEF_BAD_TIME
"E_WB0098_INST_DBDEF_BAD_TIME - Invalid database connection timeout value.\n
You did not supply a database connection timeout value or the value your have\n
specified is invalid. Please correct the problem and try again.\n"

/*%
** Explanation:
**  You forgot to supply a database connection timeout value or the one you have
**  specified is not a valid numerical value.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that you supply a valid database connection timeout value
**  in seconds. (1 minute = 60 seconds).
*/

E_WB0099_INST_OUTPUTD_NO_DIR
"E_WB0099_INST_OUTPUTD_NO_DIR - No output directories defined.\n
You have not configured any Ingres/ICE output directories. At least one\n
output directory must be configured to use Ingres/ICE. Please\n
configure an output directory now.\n"

/*%
** Explanation:
**  You do not have any Ingres/ICE output directories defined. At least
**  one directory must be configured for Ingres/ICE to execute any 
**  requests.
**
** System Status:
**  The installation program brings you to the output directory configuration
**  screen.
**
** User Action:
**  Define at least one output directory so that you may use Ingres/ICE.
*/

E_WB009A_INST_OUTPUTD_CONFSI
"E_WB009A_INST_OUTPUTD_CONFSI - Configure Web server information first.\n
You must fully configure you Web server information first before configuring\n
the option you have chosen. Please do this now.\n"

/*%
** Explanation:
**  In order for Ingres/ICE to understand exactly where your
**  Ingres/ICE output will reside, you must first provide information
**  about your Web server. This is done in the Web Server Information section
**  of the Ingres/ICE configuration program.
**
** System Status:
**  The installation program brings you to the Web server information screen.
**
** User Action:
**  Configure all necessary information on this screen before trying to 
**  configure any output directories.
*/

E_WB009B_INST_WSERVER_BADDIR
"E_WB009B_INST_WSERVER_BADDIR - Invalid configuration file directory.\n
You did not specify your Web server's configuration file directory or the one\n
you have specified does not exist. Please enter the directory where your Web\n
server configuration files reside.\n"

/*%
** Explanation:
**  You did not specify your Web server's configuration file directory or the
**  one you have specified is invalid.
**
** System Status:
**  The installation program brings you back to the Web server information
**  screen.
**
** User Action:
**  Configure your Web server's configuration file directory. Please make sure
**  that this directory location is your Web server's true configuration file
**  directory. Otherwise, the install, uninstall and operation of\n
**  Ingres/ICE will fail.
*/

/* */ E_WB009C_INST_SHUTDWN_COMP 
"The Ingres/ICE configuration program has shut down successfully.\n"

/* */ E_WB009D_INST_SHUTDWN_TITLE  
/* */ "Shut down Ingres/ICE Configuration."

/* */ E_WB009E_INST_CREATE_TUT_DB  "Creating the tutorial database '%s' . . .\n"

/* */ E_WB009F_INST_SHUTDWN_COMP2
"To restart the configuration program, please rerun \"%s\" with the required\n
parameters.\n"

/*
**	ivpxproc.c - EXECUTE PROCEDURE.
*/
E_WB00A0_BAD_PROCVAR_NAME
"E_WB00A0_BAD_PROCVAR_NAME - Invalid procedure variable name specified for \n
'ii_procvar_type%s' for database procedure '%s'. Value submitted: %s."

/*%
** Explanation:
**	A procedure parameter variable name is either missing or invalid for one.
**
** System Status:
**	All Ingres/ICE processing stops.
**
** User Action:
**      Check your HTML form. Make sure that each database procedure variable 
**      has a valid name specified as an HTML variable.
*/

/* */ E_WB0100_INST_HTML_CLOSING
"\nIf you have already installed Ingres/ICE, your tutorial can now be\n
accessed with the following URL:\n\n
       <a href=\"http://%s/%s/%s/index.html\">http://%s/%s/%s/index.html</a>\n\n"

/* */ E_WB0101_INST_CONV_RESOURCE
/* */ "Converting old output directories . . .\n"

/* */ E_WB0102_INST_CONF_WSNAME "Configured Web server name '%s' . . .\n"

E_WB0103_UNINST_TUTDB_BAD_D
"E_WB0103_UNINST_TUTDB_BAD_D - Unable to destroy the tutorial database.\n
Could not destroy the tutorial database \"%s\"."

/*%
** Explanation:
** The configuration program was unable to destroy the tutorial database.
**
** System Status:
**    The uninstall process quits.
**
** User Action:
**  Please make sure of the following:
**    - The tutorial database is not in use by someone else
**    - The tutorial database is being destroyed by the same user as it was
**      created by.
**    - Your Ingres installation is running so the database can be destroyed.
*/

/* */ E_WB0104_INST_OUTPUTD  "Output Directories"

E_WB0105_INST_BAD_CGI_DIR 
"\nE_WB0105_INST_BAD_CGI_DIR - Invalid CGI directory:\n\n
\t\t%s\n\n
This command line value will be ignored.\n\n"

/*%
** Explanation:
** You have specified an invalid or non-existing CGI directory.
**
** System Status:
**    The configuration program quits.
**
** User Action:
**  Please make sure you specifiy a valid CGI directory name on the command
**  line.
*/

/* */ E_WB0106_INST_UNCONF_WSNAME
/* */ "E_WB0106_INST_UNCONF_WSNAME - Unconfigured Web server name.\n"

/* */ E_WB0107_INST_UNCONF_CGIDIR 
/* */ "E_WB0107_INST_UNCONF_CGIDIR - Unconfigured CGI directory.\n"

/* */ E_WB0108_INST_UNCONF_CGIREF 
/* */ "E_WB0108_INST_UNCONF_CGIREF - Unconfigured CGI URL.\n"

/* */ E_WB0109_INST_LOG_OPTIONS "Request Monitoring Options"

/* */ E_WB010A_INST_LOG_TYPE "Requests To Monitor:\n"

/* */ E_WB010B_INST_LOG_SUCCESS "Successful requests"

/* */ E_WB010C_INST_LOG_SAW 
/* */ "Successful requests and requests that cause warnings" 

/* */ E_WB010D_INST_LOG_SAE 
/* */ "Successful requests and requests that cause errors"

/* */ E_WB010E_INST_LOG_ERROR "Bad requests"

/* */ E_WB010F_INST_LOG_WARNING "Requests that cause warnings"

/* */ E_WB0110_INST_LOG_WAE "Bad requests and requests that cause warnings"

/* */ E_WB0111_INST_LOG_ALL "All requests"

/* */ E_WB0112_INST_LOG_NONE "None"

/* */ E_WB0113_INST_LOG_SCR
"This is the Request Monitoring Options screen. Here, you can configure 
Ingres/ICE to monitor the results of Ingres/ICE requests.\n"

/* */ E_WB0114_INST_LOG_REQ
"There are serveral reasons why you may wish to monitor the results of\n
Ingres/ICE requests. For example, you may wish to monitor all requests\n
that result in errors or cause warnings in order to determine what caused\n
them.<P>Select the types of requests you would like to monitor. The default is\n
to monitor only bad requests.\n"

/* */ E_WB0115_INST_LOG_MON
"View monitored requests - If you would like to view the requests\n
Ingres/ICE monitored, select an option from below:\n"

/* */ E_WB0116_INST_LOGV_QRY "Monitored Requests To View:\n"

/* */ E_WB0117_INST_LOGV_L25E "The last 25 errors"

/* */ E_WB0118_INST_LOGV_L25W "The last 25 warnings"

/* */ E_WB0119_INST_LOGV_l25S "The last 25 successful requests"

/* */ E_WB011A_INST_LOGV_AERR "All errors"

/* */ E_WB011B_INST_LOGV_AWRN "All warnings"

/* */ E_WB011C_INST_LOGV_ASUC "All successful requests"

/* */ E_WB011D_INST_LOGV_ALLR "All requests results"

/* */ E_WB011E_INST_LOGV_VIEW "View Monitored Requests"

/* */ E_WB011F_INST_LOGV_LOGR "Request Result\n"

/* */ E_WB0120_INST_LOGV_LOGM "Message\n"

/* */ E_WB0121_INST_LOGV_LOGVSCR
"Requests to view - Use this screen to view the results of requests\n
Ingres/ICE has monitored.\n<UL>\n
<LI>To view monitored requests, select an option from the <strong>Monitored\n
Requests to View</strong> dropdown listbox below and select <strong>OK</strong>.\n
<LI>To remove the current list of monitored requests, select <strong>Remove
Monitored Requests</strong> and select <strong>OK</strong>.\n</UL>\n
When you are finished with this screen, select <strong>Cancel</strong>.\n"

/* */ E_WB0122_INST_LOGV_DELETE_LOG "Remove Monitored Requests\n"

/* E_WB0123_INST_LOG_ERRORF is unused */

/* E_WB0124_INST_LOG_INVALID is unused */

/* */ E_WB0125_INST_LOGV_LOGC "Client\n"

/* */ E_WB0126_INST_LOGV_LOGL "Location\n"

/* */ E_WB0127_INST_LOGV_LOGT "Timestamp\n"

/* */ E_WB0128_INST_MISCOP_CAPP "Allow Execution of Client Applications:\n"


/* */ E_WB0129_INST_SECU_EAPP_ACCEPT
"Executing standard applications from an HTML form is another powerful feature\n
of Ingres/ICE. In order for Ingres/ICE to execute them, they\n
must meet the following criteria:\n<UL>\n
<LI>Reside in your Web server's CGI directory which can be configured in\n
the <strong>Web Server Information</strong> section.\n
<LI>Return an exit value of <strong>0</strong> for <strong>SUCCESSFUL</strong>\n
executions or <strong>a number greater than 0</strong> for <strong>FAILED\n
</strong>executions.\n
<LI>Must not prompt for any input from within the program. Ingres/ICE\n
can, however, execute applications that accept input from a standard command\n
line interface.\n</UL>\n
Please examine your programs carefully before storing them in your Web\n
server's application directory. Misbehaved applications can render a\n
system\ninoperable.\n<P>\nIf you wish to allow your clients to execute\n
applications on your Web server, make sure this option is enabled below.\n" 

/* */ E_WB012A_INST_DALLW_EXEAPP
/* */ "Do not allow Web client's to execute applications on the server\n"

/* */ E_WB012B_INST_ALLOW_EXEAPP
/* */ "Allow Web client's to execute applications on the server\n"

E_WB012C_FORBIDDEN_ACTION
"E_WB012C_FORBIDDEN_ACTION - Forbidden action.\n
You have requested a forbidden Ingres/ICE option on this server."

/*%
** Explanation:
** A user has requested Ingres/ICE to execute a forbidden request on a Web
** server.
**
** System Status:
**    The request is rejected.
**
** User Action:
**  If you receive this message, please check your Ingres/ICE 
**  configuration to make sure the request you have submitted is acceptable.
*/

E_WB012D_INST_BAD_HTML_PROCESS
"E_WB012D_INST_BAD_HTML_PROCESS - Unable to process HTML tutorial files.\n
The install process is unable to create the Ingres/ICE tutorial files\n
for your platform.\n\n
Is the tutorial directory write-protected?\n
Do the tutorial files exist in your Ingres installation directory?\n\n
Please make sure the above criteria is met and retry the install process\n
again.\n"

/*%
** Explanation:
** This install program was unable to install the Ingres/ICE tutorial.
**
** System Status:
**	The install process quits.
**
** User Action:
**	Make sure that the following conditions have been met:
**		- The tutorial directory is writable by the user running your
**		  Web server.
**
**		- The tutorial files in your Ingres installation's 
**		  tutorial directory exist. Tutorial files end in ".txt".
**
**	Once these conditions are met, rerun the install program.
*/

E_WB012E_INST_NO_ICE_LIBRARY
"E_WB012E_INST_NO_ICE_LIBRARY - Unable to find Ingres/ICE shared library.\n
The install process is unable to find the Ingres/ICE shared library for\n
your platform.\n"

/*%
** Explanation:
** This install program was unable to find the Ingres/ICE shared library
** for your platform. This is a serious error.
**
** System Status:
**	The install process quits.
**
** User Action:
**	Reinstall your Ingres/ICE files from your distribution media. If
**	the problem still persist, please contact your Ingres 
**	representative.
*/

/* */ E_WB012F_INST_BEG_CFG_COMMENT
/* */ "----- Begin Ingres/ICE modification. Do not modify. -----"

/* */ E_WB0130_AGEN_ICE_CMT
/* */ "  Generated by Ingres/ICE\n  Application Name: %s\n"


/* */ E_WB0131_INST_START_MACRO_STUP
/* */ "Starting the Ingres/ICE Macro Processor Tutorial Setup . . .\n\n"

/* */ E_WB0132_INST_START_DSQL_SETUP
/* */ "Starting the Ingres/ICE Dynamic SQL Tutorial Setup . . .\n\n"


/* */ E_WB0133_INST_CPY_CFILES "Copying sample C source code . . .\n"


/* */ E_WB0134_INST_INSTALL_DSQLEXP
"<STRONG>Dynamic SQL Tutorial:</STRONG>\n<BR>\n
The dynamic SQL tutorial shows your how to create Ingres/ICE\n
applications using dynamic SQL statements and the Ingres Report-Writer\n
utility. It also shows you how to execute standard character-based\n
applications over the World-Wide Web with little or no modifcation to your\n
original program.\n<P>\n"

/* */ E_WB0135_INST_INSTALL_MCROEXP
"<STRONG>Macro Processor Tutorial:</STRONG>\n<BR>\n
The macro processor tutorial shows you how to execute multiple SQL statements\n
on a Web page. The results of these statements are automatically fomatted by\n
Ingres/ICE and inserted onto an HTML page sent to a Web client.\n<P>\n
The macro processor tutorial requires that you have a default database name\n
configured.\n<P>\n"

/* */ E_WB0136_INST_INST_MCR "Install the Macro Processor tutorial"

/* */ E_WB0137_INST_UNINST_MCR "Uninstall the Macro Processor tutorial"


/* */ E_WB0138_INST_INSTALL_RTTUTDIR
"<STRONG>Root Tutorial Directory:</STRONG>\n<BR>\n
The root tutorial directory is the root location of all Ingres/ICE\n
tutorials. This directory must be a subdirectory underneath your primary HTML\n
document directory.\n<P>\n
If you wish to install or uninstall any of the Ingres/ICE tutorials,\n
this directory must be configured.\n<P>\n
Root Tutorial Directory: <STRONG>%s%c</STRONG>\n"


/* */ E_WB0139_INST_INSTALL_SQTUTDIR
"<STRONG>Dynamic SQL Tutorial Directory:</STRONG>\n<BR>\n
The dynamic SQL tutorial directory is a subdirectory underneath the root\n
tutorial directory where the dynamic SQL tutorial is located. This directory\n
must be configured if you wish to install or uninstall the dynamic SQL\n
tutorial\n<P>\nDynamic SQL Tutorial Directory:\n"


/* */ E_WB013A_INST_INSTALL_MCTUTDIR
"<STRONG>Macro Processor Tutorial Directory:</STRONG>\n<BR>\n
The macro processor tutorial directory is a subdirectory underneath the root\n
tutorial directory where the macro processor tutorial is located. This\n
directory must be configured if you wish to install or uninstall the macro\n
processor tutorial\n<P>\nMacro Processor Tutorial Directory:\n"


/* */ E_WB013B_INST_TUT_PASSWD
"For the dynamic SQL tutorial, user 'Ingres' will be used to access the 
tutorial database. Please enter the password for user 'Ingres' below:\n"

/* */ E_WB013C_INST_TUT_ATTUNS 
/* */ "Attempting to uninstall the original tutorial (if it exists) . . .\n\n"

/* */ E_WB013D_INST_SECU_EAPP_ACEPT2
"<STRONG>Execute Applications in Web Server CGI Directory:</STRONG>\n<BR>\n"

E_WB013E_INST_DB_INVALID
"E_WB013E_INST_DB_INVALID - Invalid database name.\n
Either you did not specify a database name or the database name you have\n
specified is not valid. Please enter a valid database name.\n"

/*%
** Explanation:
** Either you forgot to specify a default database name or the database name
** you have specified is an invalid name.
**
** System Status:
**	Configuration program returns you to the Database Defaults screen.
**
** User Action:
**	Please be sure you have specified a valid database name. You may not
**	specify database 'iidbdb' as a default database name.
*/

/* */ E_WB013F_INST_MISC_OPTIONS "Miscellaneous Options"

/* */ E_WB0140_BLBST_HASH_MARK 
"\nEach hash mark represents %d bytes of saved data or part thereof.\n\n"

/* */ E_WB0141_BLBST_USAGE
"usage: %s [options] vnode::dbname imageFile\n\n
options: -t table -b blobcol -n namecol -u -w whereclause\n\n
where:\t-t the table to update\n
\t-b the name of the column to contain the blob, default icedata\n
\t-n the name of the column to contain the filename, default icedataname\n
\t-u update mode, update the rows satisfying the where clause\n
\t-w where clause, used with -u only\n\n"


/* */ E_WB0142_INST_CPY_BLOB 
/* */ "Copying images to the tutorial database '%s' . . .\n"

/* */ E_WB0143_INST_NSES_ENTRY4 "Init fn=\"init-cgi\" II_SYSTEM=\"%s\""

/* */ E_WB0144_INST_NEW_SETTINGS 
/* */ "\nPlease restart your Web server for new settings to take effect.\n\n"

/* */ E_WB0145_INST_SECU_DBOVR_ACCEP
"<STRONG>Override Macro Processor Database:</STRONG>\n<BR>\n
By allowing clients to override the Ingres/ICE default database which\n
is used by the Ingres/ICE Macro Processor, you are allowing them\n
to use any database to process a macro processor request.\n<P>\n
If the <strong>DATABASE</strong> macro processor keyword is supplied in a\n
macro statement, the database name specified will be used to process the\n
macro.\n<P>\n
If an HTML form submitted to the Web server is a request for the macro\n
processor and a database name is specified on the form, the macro processor\n
will use this database for all macro processor statements on the page to be\n
processed. The database name on the HTML form is automatically overridden by\n
the DATABASE macro processor keyword for a single macro statement.\n<P>\n 
To have complete control over what databases your clients are allowed\n
access while using the Ingres/ICE macro processor, disable this option.\n"

/* */ E_WB0146_INST_DALLW_DBOVR
"Do not allow clients to control database access with the macro processor\n"

/* */ E_WB0147_INST_ALLOW_DBOVR
"Allow clients to control database access with the macro processor\n"

/* */ E_WB0148_INST_MISCOP_DBOVR 
/* */ "Allow Clients to Override Macro Processor Database:\n"

/* */ E_WB0149_ROWS_AFFECTED "<BR>Rows involved in query: %ld"

E_WB014A_MISSING_II_SYSTEM
"<BR>The value of II_SYSTEM is not available as an environment variable.\n<P>
Please ensure that your web server makes the II_SYSTEM environment variable\n
available and re-run iceinst."
