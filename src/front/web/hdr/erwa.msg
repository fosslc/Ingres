/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	erwa.msg -	Message file for class WA
**
** Description:
**	This file defines the ICE administration and installation messages for 
**      class WA.
**
** History:
**      17-mar-97 (harpa06)
**          Created - duplicated installation messages from erwb.msg.
**      09-jun-97 (harpa06)
**          Modified E_WA003A_DB_USERID_EXPLN
**      18-jun-97 (harpa06)
**          Corrected message E_WA000B_TUTINST_CLOSING.
**      05-sep-97 (harpa06)
**          Changed message for E_WA0026_HTML_WELCOME to upcap a few words 
**          that shouldn't have been capitalized.
**      
**          Fixed grammatical errors.
**      20-oct-97 (harpa06)
**          Added E_WA00B2_TUT_WARNING.
**      29-oct-97 (harpa06)
**          Added E_WA00B3_GTWY_VRFY, E_WA00B4_GTWY_VRFY2
**      14-nov-97 (harpa06)
**          Modified E_WA0018_INSTALL_COMPLETE and E_WA0017_UNINSTALL_COMPLETE
**          to state that the install/uninstall process is complete. It does
**          not mean however, that all went well.
**      20-nov-97 (harpa06)
**          Added E_WA00B5_ATT_LAUNCH, E_WA00B6_POINT_BROWSER.
**
**          Modified E_WA0012_BAD_OPTION, E_WA0002_CMD_INTRO, 
**          E_WA0003_CMD_SYNTAX
**      12-dec-97 (harpa06)
**          Added E_WA00B7_SECU_FRCE_PASSWD through E_WA00BA_SECUOP_PASSWDRQ
**
**          Changed all references from "Macro Processor" to "Macroprocessor"
**
**          Modified E_WA003A_DB_USERID_EXPLN
**      09-jan-98 (harpa06)
**          Modified a few messages so ICE administrators users can 
**          understand directions with less confusion.
**	10-apr-98 (mcgem01)
**	    Product name change from OpenIngres to Ingres.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
*/

#define	_WA_CLASS	226

E_WA0001_INST_ERROR
"\nE_WA0001_INST_ERROR - Install/uninstall error.\n
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
**	Be sure that the above conditions are met and rerun the installation
**	utility.
*/

E_WA0002_CMD_INTRO
" This utility will allow you to configure, install and uninstall\n
Ingres/ICE components for/from your Web server.\n\n"
 
E_WA0003_CMD_SYNTAX
" Your Web server must be running under the 'ingres' user id.\n"

/* */ E_WA0004_HTML_DIR
"<STRONG>Primary HTML Directory:</STRONG>\n<BR>\n
The primary HyperText Markup Language (HTML) document directory is the\n
<strong>root</strong> directory for your HTML documents. If you do not know what this is,\n
please consult your Web server's configuration.\n<P>\n
Enter the full path of your Web server's primary HTML document directory.\n"

/* */ E_WA0005_INST_INVALID_DIR
/* */ "You have entered an invalid directory.\n\n"

/* */ E_WA0006_INST_CGI_DIR
/* */ "Enter the name of your Common Gateway Interface (CGI) directory.\n\n"

E_WA0007_WEB_SERVER
"<STRONG>Web Server Name:</STRONG>\n<BR>\n
Enter the name or IP address, including the port number of your Web server\n
(e.g. - www.yourcompany.com:80):\n"

/* */ E_WA0008_START_SETUP
/* */ "Starting the Ingres/ICE tutorial setup . . .\n\n"

/* */ E_WA0009_POPULATE
/* */ "Populating the Ingres/ICE tutorial database '%s' . . .\n"

/* */ E_WA000A_INST_CPY_EXE2CGIBIN
/* */ "\nCopying the Ingres/ICE executable . . .\n\n"

E_WA000B_TUTINST_CLOSING
"\nIf you have already installed Ingres/ICE, your tutorial can now be\n
accessed with the following URL:\n\n
       <a href=\"http://%s/%s/%s/index.html\">http://%s/%s/%s/index.html</a>\n\n"

/* */ E_WA000C_CPY_GIF 	"Copying Compuserve GIF file(s) . . .\n"

/* */ E_WA000D_CPY_JPG 	"Copying JPG file(s) . . .\n"

/* */ E_WA000E_PROCESSING_HTML 	"Processing HTML files . . .\n"

E_WA000F_INST_DIR_NOEXIST
"E_WA000F_INST_DIR_NOEXIST - A mandatory directory or file does not exist.\n"

/*%
** Explanation:
**	One of the directories that have been configured for the 
**	Ingres/ICE installation or for Ingres/ICE itself does not 
**	exist; or the installation program cannot find your Web servers'
**	configuration file.
**
** System Status:
**	If the installation is running, it is terminated.
**	If Ingres/ICE is running, processing is terminated.
**
** User Action:
**	Be sure that the directories and files specified for internet
**	communication in CBF exist.
*/

E_WA0010_BAD_DIR_CREATE
"E_WA0010_BAD_DIR_CREATE - Unable to create tutorial directory '%s.'\n" 

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
**	Be sure that you are user "ingres" when running the installation 
**	program and that the directory where you keep your HTML documents is 
**	able to have a directory created inside of it.
*/

/* */ E_WA0011_CPY_RWF 	
"Copying Report Writer report specification files . . .\n"

E_WA0012_BAD_OPTION
"\nE_WA0013_BAD_OPTION - Invalid option or missing value for: '%s'.\n\n"


/*%
** Explanation:
**	An invalid option or a missing parameter was specified on the command 
**      line. 
**
** System Status:
**	The installation program terminates.
**
** User Action:
**	Rerun the installation program and specify the proper syntax for each
**	valid option on the command line. To view the correct syntax of the
**	utility, type the name of the program on the command line.
*/

/* */ E_WA0013_INST_RM_ICE	
"\nUninstalling Ingres Internet Communication . . .\n\n"

/* */ E_WA0014_CPY_ICE
"\nCopying Internet Communication files . . .\n\n"

/* */ E_WA0015_RM_TUTORIAL "Uninstalling the %s tutorial . . .\n\n"

E_WA0016_BAD_UNINST_TUTOR
"\nE_WA0016_INST_BAD_UNINST_TUTOR - Cannot uninstall the tutorial.\n
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
**	Be sure that the following conditions have been met:
**		- The tutorial directory is writable by the "ingres" userid.
**		- The tutorial directory can be removed.
**		- All files in the tutorial directory can be removed.
**		- The tutorial database does not have a lock on it.
**              - The tutorial is not already uninstalled.
**
**	Once these conditions are met, rerun the uninstall program.
*/

/* */ E_WA0017_UNINSTALL_COMPLETE	
"\nUninstall process complete.\n\n"

/* */ E_WA0018_INSTALL_COMPLETE	
"Installation process complete.\n\n"

/* */ E_WA0019_INST_SGC_CFG_FIL_CMT
"%sAdded by Ingres/ICE. Do not modify this line and the next line.\n"

/* */ E_WA001A_MOD_CONFIG	
"Modifying Web server configuration file: \"%s\" for Web server \"%s\".\n
A backup file with with an extension of \".bak\" will be made for you . . .\n\n"

E_WA001B_CFG_FILE_OK
"E_WA001B_CFG_FILE_OK - Configuration file already modified.\n
This Web server configuration file has already been modifed. No changes will\n
be made.\n\n"

E_WA001C_CORRECT_SYNTAX
"\n Usage: iceinst [-d <CGI directory>] [-u <CGI URL>] [-s <Web server name>]\n
               [-b <Web browser> | -remote]\n\n
        CGI directory   = The full path to your Web server's CGI directory\n
                          (if not already configured)\n\n
        CGI URL         = The Uniform Resource Location (URL) to your CGI\n
                          directory (if not already configured)\n\n
        Web server name = The name of your Web server known on the internet\n
                          and/or the intranet (if not already configured)\n\n
        Web browser     = The full path and file name of your Web browser\n\n
        -remote         = To launch your Web browser from a different computer\n\n"

/* */ E_WA001D_HTML_TITLE "Ingres/ICE Configuration - "

/* */ E_WA001E_WSERVER_TYPE
"<STRONG>Supported Web Server:</STRONG>\n<BR>\n
Select a Web server you have already installed that Ingres/ICE\n
supports. If you do not know what Web server you have, please refer to your\n
Web server's documentation for more information before continuing. If you are\n
not sure after reading your documentation or your Web server is not listed\n
below, select the default type\n
<strong>%s</strong>.\n"

/* */ E_WA001F_INST_INSTALL_TYPE
"Select the operation you would like to perform:\n"

/* */ E_WA0020_INSTALL_ICEBIN
"<STRONG>Ingres/ICE Binary:</STRONG>\n<BR>\n
The Ingres/ICE binary is the heart of Ingres/ICE. This is the\n
component that is responsible for processing all Ingres/ICE Web client\n
request.\n<P>\n"

/* */ E_WA0021_ICE_CGI_DIR
"<STRONG>Primary CGI Directory:</STRONG>\n<BR>\n
Your primary Common Gateway Interface (CGI) directory is where the majority\n
of your programs executed by your Web clients are stored. Ingres/ICE\n
uses this directory to execute applications requested from an HTML form. If\n
you do not know what your primary CGI directory is, please consult your Web\n
server's configuration before continuing.\n<P>\n
Enter the full path to your primary Common Gateway Interface (CGI)\n
directory:\n"

/* */ E_WA0022_DALLW_DSQL_QUERY
/* */ "Do not accept dynamic SQL queries from HTML Pages\n"

/* */ E_WA0023_ALLOW_DSQL_QUERY
/* */ "Accept dynamic SQL queries from HTML Pages\n"

/* */ E_WA0024_SECU_DALLW_DBOVR
"Do not allow clients to control database access with the macro processor\n"

/* */ E_WA0025_SECU_ALLOW_DBOVR
"Allow clients to control database access with the macro processor\n"

/* */ E_WA0026_HTML_WELCOME
"This is the Ingres/ICE main configuration screen. If you are going to\n
install or uninstall any of the Ingres/ICE components, you must\n
have at <EM>least</EM> the following configured:\n<UL>\n
<LI>Web server information\n
<LI>At least one output directory\n
<LI>Install and/or uninstall options\n</UL>\n
If you wish to run the tutorials, you must also configure your security\n
options. Other options can be configured at your leisure.\n<P>\n
Once you have met the minimum requirements above, select <strong>Install\n
and/or\n Uninstall Selected Components</strong> to install and/or uninstall\n
the\n Ingres/ICE components you have chosen.\n<P>\n
When you are finished with the configuration program, select <strong>Close\n
Configuration Program</strong> to correctly shut down the\n
Ingres/ICE configuration program.\n" 

/* */ E_WA0027_PICK_CFG  "Select one of the following options:\n"

/* */ E_WA0028_DBDEF_TIMEOUT  "Database connection timeout value:\n"

/* */ E_WA0029_DBDEF_DBNAME  "Default database name:\n"

/* */ E_WA002A_DBDEF_USER  "Default user name:\n"

/* */ E_WA002B_SECONDS  " seconds"

/* */ E_WA002C_CGI_URL
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

E_WA002D_BAD_BROWSER
"\nE_WA002D_BAD_BROWSER - Bad Web browser:\n\n
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
**  Please make ssure to specifiy the full path and executable name of your Web
**  browser on the command line. If you do not have a Web browser, you cannot
**  install, uninstall or configure Ingres/ICE.
*/

/* */ E_WA002E_CONF_CGIDIR
"Configured Common Gateway Interface Directory '%s' . . .\n"

/* */ E_WA002F_CONF_CGIURL
"Configured Common Gateway Interface Uniform Resource Location '%s' . . .\n"

/* */E_WA0030_MAIN_SCREEN  "Main Configuration Screen"

/* */ E_WA0031_TUT_WARNING
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

/* */ E_WA0032_CFG_NSES_ENTRY1
"Init fn=\"load-modules\" shlib=\"%s\" funcs=\"%s\"" 

/* */ E_WA0033_CFG_NSES_ENTRY2
"Service fn=\"%s\" method=\"(GET|POST)\" type=\"magnus-internal/ca-ice\""

/* */ E_WA0034_CFG_NSES_ENTRY3 "type=magnus-internal/ca-ice exts=%s\n"

/* */ E_WA0035_CFG_NSES_ENTRY4 "Init fn=\"init-cgi\" II_SYSTEM=\"%s\""

E_WA0036_BAD_INPUT_DATA "E_WA0036_BAD_INPUT_DATA - Invalid data.\n
You have submitted invalid information to the configuration program. Please\n
correct the problem and try again.\n"

E_WA0037_IM_LOST "E_WA0037_IM_LOST - Nothing to do.\n
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
**  Please make sure you have specified a Ingres/ICE component to install
**  or uninstall before selecting the option to install/uninstall selected
**  products.
*/

E_WA0038_BAD_INSTALL 
"E_WA0038_BAD_INSTALL - %d error(s) detected during the install/uninstall\n
process.\n"

/* */ E_WA0039_DB_DEFAULTS "Database Defaults"

/* */ E_WA003A_DB_USERID_EXPLN
"<STRONG>Default User Name:</STRONG>\n<BR>\n
The default user name is used for an Ingres/ICE request if one is not\n
specified through your Web server's Basic authentication scheme or on an HTML\n
form. Think of the default user name as the <EM>anonymous</EM> user. As an\n
anonymous user, it is highly recommended that you only provide the user with\n
privileges common to all other users accessing your data. The default user\n
name is <strong>ingres</strong>.\n"

/* */ E_WA003B_CONFIRM_TITLE "Confirm Install/Uninstall"

/* */ E_WA003C_SETTING_CONFIRM
"Are you sure the above settings are the ones you would like to use? This will\n
be your last chance to change your mind before the install/uninstall process\n
begins.\n"

/* */ E_WA003D_INSTOP_SCR
"This is the installation options configuration screen. Here, you can select\n
any of the Ingres/ICE components you would like to install or\n
uninstall.\n"

/* */ E_WA003E_INSTALL_OPTIONS "Installation Options"

/* */ E_WA003F_OUTPUTD_NONE "No directories are configured\n"

/* */ E_WA0040_INSTALL_ICE "Install Ingres/ICE"

/* */ E_WA0041_UNINSTALL_ICE "Uninstall Ingres/ICE"

/* */ E_WA0042_DB_DEFAULTS_SCR
"This is the database defaults configuration screen. Here, you can configure:\n
<UL>\n<LI>The database connection timeout value\n
<LI>The default database name\n</UL>\n"

/* */ E_WA0043_DB_CT_EXPLAIN
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

/* */ E_WA0044_DB_NAME_EXPLAIN
"<STRONG>Default Database Name:</STRONG>\n<BR>\n
The default database name, which may also contain a nodename, is used if one\n
is not specified on an HTML form. The default is <strong>icedb</strong>.\n"

/* */ E_WA0045_END_CFG_COMMENT 
/* */ "----- End Ingres/ICE modification -----"

/* */ E_WA0046_BEG_CFG_COMMENT
/* */ "----- Begin Ingres/ICE modification. Do not modify. -----"

/* */ E_WA0047_INSTALL_DSQL "Install the dynamic SQL tutorial"

/* */ E_WA0048_UNINSTALL_DSQL "Uninstall the dynamic SQL tutorial"

/* */ E_WA0049_OUTPUTD_SCRN
"This is the Output Directories configuration screen. Here, you can configure\n
as many output directories as you would like to store client output. Each\n
output directory contains a directory label, a directory location, and a file\n
expiration time. The user running your Web server must have read and write\n
permission to all configured locations.\n"

/* */ E_WA004A_OUTPUTD_LABEL
"<STRONG>Directory Label:</STRONG>\n<BR>\n
The directory label is used to identify the location where client output is\n
stored. This label is defined in the Ingres/ICE reserved word\n
<strong>ii_output_dir</strong> on an HTML form. By using directory labels,\n
clients that view your HTML source cannot determine the physical location of\n
Ingres/ICE request output. If a client knew where your output was\n
stored, they may attempt to break into your Web server and try to access other\n
files in the same directory.\n"

/* */ E_WA004B_OTUPUTD_LABEL2
"At least one output directory must be configured:\n<UL>\n
<LI>To add a directory location, enter all information below. If you would\n
like your new location become the default, select the <strong>Use as the\n
default location</strong> checkbox.\n
<LI>To modify the attributes of a configured location, enter the directory\n
label, the attributes you wish to modify, and select <strong>Add/Modify</strong>.\n
<LI>To remove a directory location, enter the directory label and select\n
<strong>Delete</strong>.\n</UL>\n
When you are finished configuring your directories, select <strong>Cancel</strong>\n."

/* */ E_WA004C_OUTPUTD_LOCATION
"<STRONG>Directory Location:</STRONG>\n<BR>\n
A directory location is a subdirectory path underneath your primary HTML\n
directory where Ingres/ICE request output is stored. The Uniform\n
Resource Location (URL) mapping of this directory must be of the same name.\n
If this directory does not already exist, please create it now.\n"

/* */ E_WA004D_OUTPUTD_EXP_TIME
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

/* */ E_WA004E_OUTPUTD_DIRLABEL "Directory label:\n"

/* */ E_WA004F_OUTPUTD_DIRLOCAT 
/* */ "Directory location: <strong>%s%c</strong>\n"

/* */ E_WA0050_OTUPUTD_FILEEXPT "File expiration time:\n"

/* */ E_WA0051_SECU_OPTIONS_SCR
"This is the security options screen. Here, you can configure\n
Ingres/ICE options that may render your system inoperable if not\n
confgured carefully.\n"

/* */ E_WA0052_SECU_OPTIONS "Security Options"

/* */ E_WA0053_SECU_DSQL_ACCEPT
"<STRONG>Accept Dynamic SQL Queries from Web Pages:</STRONG>\n<BR>\n
Executing dynamic SQL queries from an HTML form is a powerful feature of\n
Ingres/ICE. It can be used to render a database or an Ingres\n
installation unusable if not used correctly. To have complete control over\n
what is happening to your corporate data on the internet, disable this option\n
to prevent Web clients from executing dynamic SQL queries of any kind from\n
HTML forms.\n" 

/* */ E_WA0054_SECU_EAPP_ACCEPT
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

/* */ E_WA0055_SECU_EAPP_ACEPT2
"<STRONG>Execute Applications in Web Server CGI Directory:</STRONG>\n<BR>\n"

/* */ E_WA0056_SECU_DBOVR_ACCEP
"<STRONG>Override Macroprocessor Database:</STRONG>\n<BR>\n
By allowing clients to override the Ingres/ICE default database which\n
is used by the Ingres/ICE Macroprocessor, you are allowing them\n
to use any database to process a macro processor request.\n<P>\n
If the <strong>DATABASE</strong> macro processor keyword is supplied in a\n
macro statement, the database name specified will be used to when executing\n
a macro statement.\n<P>\n
If an HTML form submitted to the Web server is a request for the macro\n
processor and a database name is specified on the form, the macro processor\n
will use this database for all macro processor statements on the page to be\n
processed. The database name on the HTML form is automatically overridden by\n
the DATABASE macro processor keyword for a single macro statement.\n<P>\n 
To have complete control over what databases your clients are allowed\n
access while using the Ingres/ICE macro processor, disable this option.\n"

/* */ E_WA0057_WSERVER_INFO "Web Server Information"

/* */ E_WA0058_WSERVER_INFO_SCR
"This is the Web server information configuration screen. Here, you can\n
configure the bulk of the information Ingres/ICE needs to know about\n
your Web server. What Ingres/ICE needs to know here is:\n<UL>\n
<LI>The type of Web server you have\n
<LI>Your Web server's configuration directory\n
<LI>The name and port number of your Web server known on the internet\n
<LI>Your Web server's primary HTML directory\n
<LI>Your Web server's primary Common Gateway Interface (CGI) directory\n
<LI>The Resource Location (URL) mapping to your primary CGI directory\n</UL>\n"

/* */ E_WA0059_WSERVER_CFG_DIR
"<STRONG>Web Server Configuration Directory:</STRONG>\n<BR>\n
Your Web server's configuration directory is the location where your critical\n
Web server information is stored. To have your Web server communicate\n
directly with Ingres, the Ingres/ICE configuration program needs\n
to know where this directory is located. If you do not know where your\n
configuration directory is located, please refer to your Web server's\n
configuration before continuing.\n<P>\n
Enter the path to your Web server's configuration directory. This entry will\n
be ignored if <strong>%s</strong> is selected.\n<BR>\n"

/* */ E_WA005A_INSTALL_RESULTS "Install/Uninstall Results"

/* */ E_WA005B_CFGOP_HDR_OUTDIR "Output Directories"

/* */ E_WA005C_CFGOP_LABEL "Label:\n"

/* */ E_WA005D_CFGOP_LOCATION "Location (subdirectory):\n"

/* */ E_WA005E_CFGOP_FEXPTIME "File Expiration Time:\n"

/* */ E_WA005F_DBDEF_CONNT "Connection Timeout:\n"

/* */ E_WA0060_DBDEF_DBNAME "Database Name:\n"

/* */ E_WA0061_DBDEF_USERNAME "Default User Name:\n"

/* */ E_WA0062_SECUOP_DSQL "Accept Dynamic SQL Queries:\n"

/* */ E_WA0063_WSOPT_CONF_WS "Configured Web Server:\n"

/* */ E_WA0064_WSOPT_CONF_WSDIR "Configuration Directory:\n"

/* */ E_WA0065_WSOPT_SERVERNAME "Server Name:\n"

/* */ E_WA0066_WSOPT_MAINDOCDIR "Main Document Directory:\n"

/* */ E_WA0067_WSOPT_CLNTAPPDIR 
"Common Gateway Interface (CGI) Directory:\n"

/* */ E_WA0068_WSOPT_CLNTAPPURL 
"Uniform Resource Location (URL) to CGI directory:\n"

E_WA0069_INSTALL_TUTDB "Tutorial database name:\n"

E_WA006A_INSTALL_TUTDREXP
"<STRONG>Dynamic SQL Tutorial Database Name:</STRONG>\n<BR>\n
If you choose to install the Dynamic SQL tutorial, a database\n
will be used as the Dynamic SQL tutorial database. This database is\n
populated with the information required to run the tutorial. The database name\n
you enter here will be the database used for the tutorial. To create the\n
database in another installation, be sure to include it's nodename when\n
specifying the database.\n<P>\nThe default database name is <strong>icedb\n
</strong>.\n"

E_WA006B_OUPUTD_NOEXP 
"E_WA006B_OUTPUTD_NOEXP - Invalid expiration time.\n
You did not supply a file expiration time or the one you have supplied is not\n
valid. Please correct the problem and try again.\n"

/*%
** Explanation:
**  The file expiration time you have supplied is invalid or you forgot to 
**  supply one.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that the file expiration time you are about to submit to
**  the installation program is correct before submitting it.
*/

E_WA006C_OUTPUTD_BADDIR
"E_WA006C_OUTPUTD_BADDIR - Invalid directory location.\n
You did not supply a directory location or the one you have supplied is not\n
valid. Please correct the problem and try again.\n"

/*%
** Explanation:
**  The directory location you have supplied is invalid or you forgot to 
**  supply one.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that the directory location are about to submit to
**  the installation program is a valid subdirectory off of your primary HTML
**  directory.
*/

E_WA006D_OUTPUTD_NODLBL
"E_WA006D_OUTPUTD_NODLBL - No directoy label specified.\n
Either you did not supply a directory label or the one you have specified does\n
not exist. Please correct the problem and try again.\n"

/*%
** Explanation:
**  You forgot to supply a directory label for the output directory you wish to
**  use with ICE or the one you have specified does not exist.
**
** System Status:
**  The installation program brings you back to the original screen.
**
** User Action:
**  Please make sure that you supply a valid directory label.
*/

E_WA006E_DBDEF_BAD_TIME
"E_WA006E_DBDEF_BAD_TIME - Invalid database connection timeout value.\n
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

E_WA006F_OUTPUTD_NO_DIR
"E_WA006F_OUTPUTD_NO_DIR - No output directories defined.\n
You have not configured any Ingres/ICE output directories. At least one\n
output directory must be configured to use Ingres/ICE. Please\n
configure an output directory now.\n"

/*%
** Explanation:
**  You do not have any Ingres/ICE output directories defined. At least
**  one directory must be configured for Ingres/ICE to work.
**
** System Status:
**  The installation program brings you to the output directory configuration
**  screen.
**
** User Action:
**  Define at least one output directory so that you may use Ingres/ICE.
*/

E_WA0070_OUTPUTD_CONFSI
"E_WA0070_OUTPUTD_CONFSI - Configure Web server information first.\n
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

E_WA0071_WSERVER_BADDIR
"E_WA0071_INST_WSERVER_BADDIR - Invalid configuration file directory.\n
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
**  Configure your Web server's configuration file directory. Please be sure
**  that this directory location is your Web server's true configuration file
**  directory. Otherwise, the install, uninstall and operation of\n
**  Ingres/ICE will fail.
*/

/* */ E_WA0072_SHUTDWN_COMP 
"The Ingres/ICE configuration program has shut down successfully.\n"

/* */ E_WA0073_SHUTDWN_COMP2
"To restart the configuration program, please rerun \"%s\" with the required\n
parameters.\n"

/* */ E_WA0074_SHUTDWN_TITLE  
/* */ "Shut down Ingres/ICE Configuration."

/* */ E_WA0075_CREATE_TUT_DB  "Creating the tutorial database '%s' . . .\n"

/* */ E_WA0076_INST_HTML_CLOSING
"\nIf you have already installed Ingres/ICE, your tutorial can now be\n
accessed with the following URL:\n\n
       <a href=\"http://%s/%s/%s/index.html\">http://%s/%s/%s/index.html</a>\n\n"

/* */ E_WA0077_CONV_RESOURCE
/* */ "Converting old output directories . . .\n"

/* */ E_WA0078_CONF_WSNAME "Configured Web server name '%s' . . .\n"

E_WA0079_TUTDB_BAD_DESTROY
"E_WA007A_TUTDB_BAD_DESTROY - Unable to destroy the tutorial database.\n
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

/* */ E_WA007B_OUTPUTD "Output Directories"

E_WA007C_BAD_CGI_DIR 
"\nE_WA007C_BAD_CGI_DIR - Invalid CGI directory:\n\n
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

/* */ E_WA007D_UNCONF_WSNAME
/* */ "E_WA007D_UNCONF_WSNAME - Unconfigured Web server name.\n"

/* */ E_WA007E_UNCONF_CGIDIR 
/* */ "E_WA007E_UNCONF_CGIDIR - Unconfigured CGI directory.\n"

/* */ E_WA007F_UNCONF_CGIREF 
/* */ "E_WA007F_UNCONF_CGIREF - Unconfigured CGI URL.\n"

/* */ E_WA0080_LOG_OPTIONS "Request Monitoring Options"

/* */ E_WA0081_LOG_TYPE "Requests To Monitor:\n"

/* */ E_WA0082_LOG_SUCCESS "Successful requests"

/* */ E_WA0083_LOG_SAW 
/* */ "Successful requests and requests that cause warnings" 

/* */ E_WA0084_LOG_SAE 
/* */ "Successful requests and requests that cause errors"

/* */ E_WA0085_LOG_ERROR "Bad requests"

/* */ E_WA0086_LOG_WARNING "Requests that cause warnings"

/* */ E_WA0087_LOG_WAE "Bad requests and requests that cause warnings"

/* */ E_WA0088_LOG_ALL "All requests"

/* */ E_WA0089_LOG_NONE "None"

/* */ E_WA008A_LOG_SCR
"This is the Request Monitoring Options screen. Here, you can configure 
Ingres/ICE to monitor the results of Ingres/ICE requests.\n"

/* */ E_WA008B_LOG_REQ
"There are serveral reasons why you may wish to monitor the results of\n
Ingres/ICE requests. For example, you may wish to monitor all requests\n
that result in errors or cause warnings in order to determine what caused\n
them.<P>Select the types of requests you would like to monitor. The default is\n
to monitor only bad requests.\n"

/* */ E_WA008C_INST_LOG_MON
"View monitored requests - If you would like to view the requests\n
Ingres/ICE monitored, select an option from below:\n"

/* */ E_WA008D_LOGV_QRY "Monitored Requests To View:\n"

/* */ E_WA008E_LOGV_LOGR "Request Result\n"

/* */ E_WA008F_LOGV_LOGM "Message\n"

/* */ E_WA0090_LOGV_LOGVSCR
"Requests to view - Use this screen to view the results of requests\n
Ingres/ICE has monitored.\n<UL>\n
<LI>To view monitored requests, select an option from the <strong>Monitored\n
Requests to View</strong> dropdown listbox below and select <strong>OK</strong>.\n
<LI>To remove the current list of monitored requests, select <strong>Remove
Monitored Requests</strong> and select <strong>OK</strong>.\n</UL>\n
When you are finished with this screen, select <strong>Cancel</strong>.\n"

/* */ E_WA0091_LOGV_RM_LOG "Remove Monitored Requests\n"

/* E_WA0092_LOG_ERRORF is unused */

/* E_WA0093_LOG_INVALID is unused */

/* */ E_WA0094_LOGV_LOGC "Client\n"

/* */ E_WA0095_LOGV_LOGL "Location\n"

/* */ E_WA0096_LOGV_LOGT "Timestamp\n"

/* */ E_WA0097_SECUOP_CAPP "Allow Execution of Client Applications:\n"

/* */ E_WA0098_DALLW_EXEAPP
/* */ "Do not allow Web client's to execute applications on the server\n"

/* */ E_WA0099_ALLOW_EXEAPP
/* */ "Allow Web client's to execute applications on the server\n"

E_WA009A_PROCESS_HTML_BAD
"E_WA009A_PROCESS_HTML_BAD - Unable to process HTML tutorial files.\n
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
**	Be sure that the following conditions have been met:
**		- The tutorial directory is writable by the user running your
**		  Web server.
**
**		- The tutorial files in your Ingres installation's 
**		  tutorial directory exist. Tutorial files end in ".txt".
**
**	Once these conditions are met, rerun the install program.
*/

E_WA009B_NO_ICE_LIBRARY
"E_WA009B_NO_ICE_LIBRARY - Unable to find Ingres/ICE shared library.\n
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

/* */ E_WA009C_INSTALL_DSQLEXP
"<STRONG>Dynamic SQL Tutorial:</STRONG>\n<BR>\n
The dynamic SQL tutorial shows your how to create Ingres/ICE\n
applications using dynamic SQL statements and the Ingres Report-Writer\n
utility. It also shows you how to execute standard character-based\n
applications over the World-Wide Web with little or no modifcation to your\n
original program.\n<P>\n"

/* */ E_WA009D_INSTALL_MACROEXP
"<STRONG>Macroprocessor Tutorial:</STRONG>\n<BR>\n
The macro processor tutorial shows you how to execute multiple SQL statements\n
on a Web page. The results of these statements are automatically fomatted by\n
Ingres/ICE and inserted onto an HTML page sent to a Web client.\n<P>\n
The macro processor tutorial requires that you have a default database name\n
configured.\n<P>\n"

/* */ E_WA009E_INSTALL_MACROP "Install the Macroprocessor tutorial"

/* */ E_WA009F_UNINSTALL_MCRP "Uninstall the Macroprocessor tutorial"


/* */ E_WA00A0_INSTALL_RTTUTDIR
"<STRONG>Root Tutorial Directory:</STRONG>\n<BR>\n
The root tutorial directory is the root location of all Ingres/ICE\n
tutorials. This directory must be a subdirectory underneath your primary HTML\n
document directory.\n<P>\n
If you wish to install or uninstall any of the Ingres/ICE tutorials,\n
this directory must be configured.\n<P>\n
Root Tutorial Directory: <STRONG>%s%c</STRONG>\n"


/* */ E_WA00A1_INSTALL_SQTUTDIR
"<STRONG>Dynamic SQL Tutorial Directory:</STRONG>\n<BR>\n
The dynamic SQL tutorial directory is a subdirectory underneath the root\n
tutorial directory where the dynamic SQL tutorial is located. This directory\n
must be configured if you wish to install or uninstall the dynamic SQL\n
tutorial\n<P>\nDynamic SQL Tutorial Directory:\n"


/* */ E_WA00A2_INSTALL_MCTUTDIR
"<STRONG>Macroprocessor Tutorial Directory:</STRONG>\n<BR>\n
The macro processor tutorial directory is a subdirectory underneath the root\n
tutorial directory where the macro processor tutorial is located. This\n
directory must be configured if you wish to install or uninstall the macro\n
processor tutorial\n<P>\nMacroprocessor Tutorial Directory:\n"


/* */ E_WA00A3_TUT_PASSWD
"For the dynamic SQL tutorial, user 'Ingres' will be used to access the 
tutorial database. Please enter the password for user 'Ingres' below:\n"

/* */ E_WA00A4_TUT_ATTUNS 
/* */ "Attempting to uninstall the original tutorial (if it exists) . . .\n\n"

E_WA00A5_DB_INVALID
"E_WA00A5_DB_INVALID - Invalid database name.\n
Either you did not specify a database name or the database name your have\n
specified is not valid. Please enter a valid database name.\n"

/*%
** Explanation:
** Either you forgot to specify a default database name of the database name
** you have specified is an invalid name.
**
** System Status:
**	Configuration program returns you to the Database Defaults screen.
**
** User Action:
**	Please be sure you have specified a valid database name. You may not
**	specify database iidbdb as a default database name.
*/

/* */ E_WA00A6_INST_MISC_OPTIONS "Miscellaneous Options"

/* */ E_WA00A7_CPY_BLOB 
/* */ "Copying images to the tutorial database '%s' . . .\n"

/* */ E_WA00A8_SECUOP_DBOVR 
/* */ "Allow Clients to Override Macroprocessor Database:\n"

/* */ E_WA00A9_START_MACRO_SETUP
/* */ "Starting the Ingres/ICE Macroprocessor Tutorial Setup . . .\n\n"

/* */ E_WA00AA_START_DSQL_SETUP
/* */ "Starting the Ingres/ICE Dynamic SQL Tutorial Setup . . .\n\n"

/* */ E_WA00AB_CPY_CFILES "Copying sample C source code . . .\n"

/* */ E_WA00AC_LOGV_AERR "All errors"

/* */ E_WA00AD_LOGV_AWRN "All warnings"

/* */ E_WA00AE_LOGV_ASUC "All successful requests"

/* */ E_WA00AF_LOGV_ALLR "All requests results"

/* */ E_WA00B0_LOGV_VIEW "View Monitored Requests"

/* */ E_WA00B1_NEW_SETTINGS 
/* */ "\nPlease restart your Web server for new settings to take effect.\n\n"

/* */ E_WA00B2_TUT_WARNING
"<strong>\n<font color=\"red\">Warning:</font> If you have chosen to use the\n
default database as a tutorial database, the tutorial database will remain\n
untouched to preseve any data added to the default database from another\n
application.\n</strong>\n<P>\n\n"

/* */ E_WA00B3_GTWY_VRFY
"\nPlease verify that the tutorial installation has completed successfully\n
by going into the database you have selected and execute a SELECT on the\n
HARDWARE table.\n<P>\n"

/* */ E_WA00B4_GTWY_VRFY2
"\nPlease verify that the tutorial installation has completed successfully\n
by going into the database you have selected and execute the HELP statement.\n
You should see a table named 'bookauthor.'<P>\n"

/* */ E_WA00B5_ATT_LAUNCH "\nAttempting to launch:\n\n   %s\n"

/* */ E_WA00B6_POINT_BROWSER 
"\nTo continue, please point your Web browser to:\n\n   %s\n"

/* */ E_WA00B7_SECU_FRCE_PASSWD
"<STRONG>Require All Clients to Submit Passwords:</STRONG>\n<BR>\n
By forcing clients to submit passwords along with their requests, the chances\n
of a client attempting to use another client's identification to process\n
their request is greatly reduced.\n<P>\nFor example, if user ID \"joe\" has\n
security privileges and you are requiring your clients to submit passwords\n
with their requests, someone trying to use user ID \"joe\" without the\n
accompanying password will not have access to any data.\n<P>
To reduce the chances of a Web client trying to use someone elses\n
identification\n to access your data, disable this option.\n"

/* */ E_WA00B8_SECU_REQUR_PASSWD
"Require clients to submit a password for requests.\n"

/* */ E_WA00B9_SECU_NREQR_PASSWD
"Do not require clients to submit a password for requests.\n"

/* */ E_WA00BA_SECUOP_PASSWDRQ 
/* */ "Require Clients to Supply a Password for Requests:\n"
