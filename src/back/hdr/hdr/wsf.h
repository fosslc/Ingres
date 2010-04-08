/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: wsf.h
**
** Description:
**      Constant definitions and macros used within the wsf facility.
**
** History:
**      ??-Mar-1998 (marol01)
**          Created.
**      21-Sep-98 (fanra01)
**          Add history and reformatted.  Modified the config.dat parameters.
**      25-Sep-1998 (fanra01)
**          Corrected incomplete last line.
**      01-Oct-98 (fanra01)
**          Update the cookie string to reflect dll name change.
**      06-Oct-1998 (fanra01)
**          Removed closing tag of select.
**          Add open, close parenthases and plus characters.
**          Fix typo for dsql config parameter.
**      08-Oct-98 (fanra01)
**          Change the starting node for query parsing to 0.
**      02-Nov-98 (fanra01)
**          Add variables for remote address and remote host.
**      03-Nov-98 (fanra01)
**          Add session group to replace application.
**      13-Nov-98 (fanra01)
**          Bug 94157
**          Incorrect content type string returned from netscape server.
**          Update case of Content-Type to content-type as netscape is
**          case sensitive.
**      07-Jan-1999 (fanra01)
**          Add HTML variable for authentication type.  Used for web users to
**          allow password checking not from repository but from another
**          source.
**      07-Apr-1999 (fanra01)
**          Add CONF_OUTPUT_DEFAULT for defining the default ICE 2.0 location.
**      13-May-1999 (fanra01)
**          Add ii_usernum define.
**      17-Aug-1999 (fanra01)
**          Bug 98429
**          Separate newline from cookie and add expire time string.
**      25-Jan-2000 (fanra01)
**          Bug 100132
**          Add string definition for cookie max age.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add STR_DEFAULT_SCHEME.
**      03-Aug-2000 (fanra01)
**          Bug 100527
**          Add WSF_TEMP to mark a document as temporary. i.e. in the memory
**          version of the repository but not in the database version.
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Add string constants for content type and add a formatting type
**          for XML support.
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add row_set and set_size configuration parameter strings.
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add the definition for the ice_var function.
**      20-Jun-2001 (fanra01)
**          Sir 103096
**          Add XML format types and strings.
**          XML_ICE_TYPE_PDATA      XML option.  XML literals not treated.
**          XML_ICE_TYPE_XML        TYPE option. XML literals converted.
**          XML_ICE_TYPE_XML_PDATA  TYPE option. XML literals not treated.
**      20-Nov-2001 (fanra01)
**          Bug 106434
**          Modified STR_COOKIE to take optional value assignment by 
**          parameterizing the format string.
**      23-Jan-2003 (hweho01)
**          For hybrid build 32/64 on AIX, the 64-bit shared lib   
**          oiddi needs to have suffix '64' to reside in the   
**          the same location with the 32-bit one, due to the alternate 
**          shared lib path is not available in the current OS.
**      20-Aug-2003 (fanra01)
**          Bug 110747
**          Increase delimter define to take account of include space
**          character.
**          String is "var1='val1', var2='val2'"
**      25-Jan-2005 (hweho01)
**          Defined OPEN_INGRES_DLL to "oiddi64" for 64-bit oiddi   
**          shared library on AIX platform.   
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
#ifndef WSF_INCLUDED
#define WSF_INCLUDED

#include <ddfcom.h>

# if defined(any_aix) && defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
#define OPEN_INGRES_DLL	ERx("oiddi64")
#else
#define OPEN_INGRES_DLL	ERx("oiddi")
#endif  /* aix  */

#define IsSeparator(X)      (X == ' ' || X == '\t' || X == 10 || X == 13)
#define IsAlpha(X)          ((X >= 'A' && X <= 'Z') || (X >= 'a' && X <= 'z'))
#define IsHexa(X)           ((X >= '0' && X <= '9') || \
                             (X >= 'A' && X <= 'F') || \
                             (X >= 'a' && X <= 'f'))
#define IsDigit(X)          (X >= '0' && X <= '9')
#define AddHexa(X, Y)       (X * 16) + ((IsDigit(Y)) ?  (Y - '0') : \
                                ((Y >= 'a' && Y <= 'f') ? (Y - 'a') : (Y - 'A')) + 10);
#define WSF_CVNA(X, Y, Z)   {   X = GAlloc((PTR*)&Z, NUM_MAX_INT_STR, FALSE); \
                                if (X == GSTAT_OK)	\
                                    CVna(Y, Z); }
#define WSF_CVLA(X, Y, Z)	{   X = GAlloc((PTR*)&Z, NUM_MAX_INT_STR, FALSE); \
                                if (X == GSTAT_OK)	\
                                    CVla(Y, Z); }

#define PURGE_SPACE(X)      { \
                                u_i4 length; \
                                char *tmp; tmp = X; \
                                length = STlength(X); \
                                while (tmp && *tmp && IsSeparator(*tmp)) \
                                { \
                                    length--; \
                                    tmp++; \
                                } \
                                MECOPY_VAR_MACRO(X, length, tmp); \
                                if (length > 0) \
                                { \
                                    tmp += --length; \
                                    while (length > 0 && IsSeparator(*tmp)) tmp--; \
                                    *tmp = EOS; \
                                } \
                            }
#define SYSTEM_ID                   0

# define ON                         ERx("ON")
# define OFF                        ERx("OFF")

# define VARIABLE_MARK              ':'
# define CHAR_URL_SEPARATOR         '/'
# define CHAR_SINGLEQUOTE           '\''
# define CHAR_FILE_SUFFIX_SEPARATOR '.'

# define NUM_MIN_RW_PARAM           1
# define STR_GATEWAY_SEPARATOR      ERx("/")
# define STR_ICE_REPORT_PREFIX      ERx("ir")
# define STR_ICE_MACRO_PREFIX       ERx("im")
# define STR_ICE_CLAPP_PREFIX       ERx("ica")
# define STR_RW_DATABASE_PARAM      ERx("database = %S, ")
# define STR_RW_USER_PARAM          ERx("user = %S, ")
# define STR_RW_FLAGS_PARAM         ERx("flags = %S, ")
# define STR_RW_FILE_PARAM          ERx("file = %S, ")
# define STR_RW_SRCFILE_PARAM       ERx("sourcefile = %S, ")
# define STR_RW_REPNAME_PARAM       ERx("report = %S, ")
# define STR_RW_MISC_PARAM          ERx("param = %S, ")

# define STR_ING_RW_FLAGS           ERx("-s +#:_=")

# define STR_ING_RW_UTEXE_NAME      ERx("report")

# define STR_LOGFILE_EXT            ERx("log")
# define STR_HTML_MIME_TYPE         ERx("text/html")
# define STR_IMAGE_MIME_TYPE        ERx("image")
# define STR_HTML_EXT               ERx("html")
# define STR_TYPE_BASE              ERx("text")

# define STR_UNKNOWN_USER           ERx("(unknown user)")
# define STR_UNKNOWN_ADDR           ERx("(unknown client address)")
# define STR_UNKNOWN_REPORT         ERx("Unknown report")
# define STR_HTTP_PREFIX            ERx("http://")
# define STR_DEFAULT_SCHEME         ERx("http")

# define STR_DEF_BINARY_PREFIX      ERx("ice")

# define STR_ICE_LOG_FILENAME       ERx("ice.log")
# define STR_ICE_LOG_SUBDIR         ERx("files")

/* Log file message filters */
# define STR_ICE_SUCCESS_MSG_FILTER ERx ("Success: ")
# define STR_ICE_ERROR_MSG_FILTER   ERx ("Error: ")
# define STR_ICE_WARNING_MSG_FILTER ERx ("Warning: ")

# define ERROR_LVL_WARNING          1
# define ERROR_LVL_ERROR            2

/* Authentication Mehtods */
# define STR_ICE_AUTH               ERx("ICE")
# define STR_OS_AUTH                ERx("OS")

/* Config.dat parameters. Note that these require that the host name
** is substituted (using sprintf) for %s. %d is number ranging from 1 to
** infinity.
*/
# define CONF_STR_SIZE              256

# define CONF_REPOSITORY            ERx("!.dictionary_name")
# define CONF_DLL_NAME              ERx("!.dictionary_driver")
# if defined(any_aix) && defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
# define CONF_DLL64_NAME            ERx("!.dictionary_driver64")
# endif   /* aix */
# define CONF_NODE                  ERx("!.dictionary_node")
# define CONF_CLASS                 ERx("!.dictionary_class")
# define CONF_RIGHT_SIZE            ERx("!.rights_table")
# define CONF_USER_SIZE             ERx("!.users_table")
# define CONF_WSS_PRIV_USER         ERx("!.privilege_user")
# define CONF_WSS_CASE_USER         ERx("!.ignore_user_case")
# define CONF_UNIT_SIZE             ERx("!.units_table")
# define CONF_DOCUMENT_SIZE         ERx("!.documents_table")
# define CONF_LOCATION_SIZE         ERx("!.locations_table")
# define CONF_EXTENSION_SIZE        ERx("!.extensions_table")
# define CONF_FILE_SIZE             ERx("!.files_table")
# define CONF_WCS_TIMEOUT           ERx("!.req_timeout")
# define CONF_USER_SESSION          ERx("!.user_session_table")
# define CONF_SYSTEM_TIMEOUT        ERx("!.system_timeout")
# define CONF_SESSION_TIMEOUT       ERx("!.db_conn_timeout")
# define CONF_DOCSESSION_SIZE       ERx("!.docsession_table")
# define CONF_WPS_TIMEOUT           ERx("!.sess_timeout")
# define CONF_WPS_BLOCK_SIZE        ERx("!.block_size")
# define CONF_WPS_BLOCK_MAX         ERx("!.block_count")
# define CONF_SVRVAR_SIZE           ERx("!.vars_table")
# define CONF_APPLICATION_SIZE      ERx("!.apps_table")
# define CONF_CLIENT_LIB            ERx("!.client_lib")
# define CONF_OUTPUT_DEFAULT        ERx("!.dir_default_location")
# define CONF_ROW_SET_MIN           ERx("!.row_set")
# define CONF_SET_SIZE_MAX          ERx("!.set_size")

/* installation
# define CONF_CONFIG_WS         ERx("ii.%s.ice.config_ws")
# define CONF_CONFIG_WS_DIR     ERx("ii.%s.ice.config_ws_dir")
# define CONF_BIN_REF           ERx("ii.%s.ice.cgi-bin_ref")
# define CONF_FORCE_PASSWORD    ERx("ii.%s.ice.security.force_passwd")
# define CONF_OUTPUT_DIR_LOC		ERx("ii.%s.ice.rw_dir%s_location")
# define CONF_OUTPUT_DIR_TIME		ERx("ii.%s.ice.rw_dir%s_time")
# define CONF_OUTPUT_LABEL      ERx("ii.%s.ice.dir%d_label")
# define CONF_OUTPUT_LOC        ERx("ii.%s.ice.dir%d_location")
# define CONF_OUTPUT_TIME       ERx("ii.%s.ice.dir%d_time")
*/

/* ICE 2.0 */
# define CONF_HTML_HOME             ERx("!.html_home")
# define CONF_BIN_DIR               ERx("!.app_dir")
# define CONF_ALLOW_DYN_SQL         ERx("!.allow_dsql")
# define CONF_ALLOW_EXE_APP         ERx("!.allow_exeapp")
# define CONF_ALLOW_DB_OVR          ERx("!.allow_dbovr")
# define CONF_OUTPUT_DIR_LOC        ERx("<rw_dir%s_location>")
# define CONF_DEFAULT_USERID        ERx("!.default_dbuser")
# define CONF_DEFAULT_DATABASE      ERx("!.default_database")
# define CONF_CONN_TIMEOUT          ERx("!.db_conn_timeout")

/* Configuration variable names */
#define CONF_DRIVER                 ERx("Driver_")

/* WEB_TYPE */
#define WSF_HTML_TYPE               ERx("htm")
#define WSF_GENERIC_TYPE            ERx("*")
#define WSF_LOG_SUFFIX              ERx("log")
#define WSF_ERROR_SUFFIX            ERx("err")
#define WSF_ICE_SUFFIX              ERx("ice")

# ifdef VMS
#  define STR_CMDLINE_QUOTE         ERx("'")
#  define CHAR_PATH_SEPARATOR       '.'
# else
#  define STR_CMDLINE_QUOTE         ERx("'")
#  ifdef NT_GENERIC
#   define CHAR_PATH_SEPARATOR      '\\'
#  else
#   define CHAR_PATH_SEPARATOR      '/'
#  endif
# endif

# define CHAR_URL_SEPARATOR         '/'
# define CHAR_SINGLEQUOTE           '\''
# define CHAR_PLUS                  '+'

# define CHAR_FILE_SUFFIX_SEPARATOR '.'

# define STR_FILE_SUFFIX_SEPARATOR  ERx(".")
# define STR_QUOTE                  ERx("\"")
# define STR_BACKQUOTE              ERx("`")
# define STR_SINGLEQUOTE            ERx("'")
# define STR_PARAM_SEPARATOR        ERx(", ")
# define STR_EQUALS_SIGN            ERx("=")
# define STR_MINUS_SIGN             ERx("-")
# define STR_COLON                  ERx(":")
# define STR_EMPTY                  ERx("")
# define STR_OBRACE                 ERx("[")
# define STR_CBRACE                 ERx("]")
# define STR_OPAREN                 ERx("(")
# define STR_CPAREN                 ERx(")")
# define STR_SPACE                  ERx(" ")
# define STR_NEWLINE                ERx("\n")
# define STR_CRLF                   ERx("\r\n")
# define STR_COMMA                  ERx(",")
# define STR_QUESTION_MARK          ERx("?")
# define STR_SEMICOLON              ERx(";")
# define STR_PERIOD                 ERx(".")
# define STR_PLUS                   ERx("+")
# define SPACE                      ' '

# define STR_ERROR_MARK             ERx(" ? ")

# define TAG_PLAIN                  ERx("<PLAINTEXT>\n")
# define TAG_END_PLAIN              ERx("\n")
# define TAG_HTML                   ERx("<HTML>\n")
# define TAG_END_HTML               ERx("</HTML>\n")
# define TAG_HEAD_TITLE             ERx("<HEAD><TITLE>")
# define TAG_END_HEAD_TITLE         ERx("</TITLE></HEAD>\n")
# define TAG_BODY                   ERx("<BODY>\n")
# define TAG_END_BODY               ERx("</BODY>\n")
# define TAG_HEADER1                ERx("<H1><CENTER>\n")
# define TAG_END_HEADER             ERx("</CENTER></H1>\n")
# define TAG_HORZ_RULE              ERx("<HR size=5>\n")
# define TAG_PARAGRAPH              ERx("<P>\n")
# define TAG_PARAGRAPH_BREAK        ERx("<BR>\n")
# define TAG_PREFORM                ERx("<PRE>\n")
# define TAG_END_PREFORM            ERx("</PRE>\n")
# define TAG_HTML_DTD               ERx("<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n")
# define TAG_COMMENT_BEGIN          ERx("<!--")
# define TAG_COMMENT_END            ERx("-->")
# define TAG_TABLE                  ERx("<TABLE ")
# define TAG_END_TABLE              ERx("</TABLE>\n")
# define TAG_TABROW                 ERx("<TR>")
# define TAG_END_TABROW             ERx("</TR>\n")
# define TAG_TABHEAD                ERx("<TH>")
# define TAG_END_TABHEAD            ERx("</TH>")
# define TAG_TABCELL                ERx("<TD>")
# define TAG_END_TABCELL            ERx("</TD>")
# define TAG_SELECT                 ERx("<SELECT ")
# define TAG_END_SELECT             ERx("</SELECT>\n")
# define TAG_CENTER                 ERx("<CENTER>\n")
# define TAG_END_CENTER             ERx("</CENTER>\n")
# define TAG_OPTION                 ERx("<OPTION>\n")
# define TAG_END_OPTION             ERx("</OPTION>\n")
# define TAG_IMGSRC                 ERx("<IMG SRC=\"")
# define TAG_END_IMGSRC             ERx("\">")
# define TAG_STRICT_END_IMGSRC      ERx("\"/>")
# define END_TAG                    ERx(">")
# define TAG_STRONG                 ERx("<STRONG>")
# define TAG_END_STRONG             ERx("</STRONG>")
# define TAG_LINK                   ERx("<A HREF=\"")
# define TAG_END_LINK_URL           ERx("\">")
# define TAG_END_LINK               ERx("</A>")
# define ATTR_BORDER                ERx("border ")
# define ATTR_DEF_CELLSPACING       ERx("cellspacing=2 ")
# define STR_NONE                   ERx("<none>")
# define STR_DEFAULT                ERx(" (default)")
# define STR_WPS_BINARY             ERx("binary data")

# define ING_DTD_TABLE_BEGIN        ERx("<table>")
# define ING_DTD_TABLE_END          ERx("</table>")
# define ING_DTD_TABLE_OPEN         ERx("<table")
# define ING_DTD_RESULTS_BEGIN      ERx("<resultset>")
# define ING_DTD_RESULTS_OPEN       ERx("<resultset")
# define ING_DTD_RESULTS_END        ERx("</resultset>")
# define ING_DTD_ROW_BEGIN          ERx("<row>")
# define ING_DTD_ROW_END            ERx("</row>")
# define ING_DTD_ROW_OPEN           ERx("<row")
# define ING_DTD_COL_BEGIN          ERx("<column>")
# define ING_DTD_COL_END            ERx("</column>")
# define ING_DTD_COL_OPEN           ERx("<column")
# define ING_DTD_COL_ATTR_OPEN      ERx("<column column_name=\"")
# define ING_DTD_COL_ATTR_CLOSE     ERx("\">")

# define HVAR_APPLICATION           ERx("ii_application")
# define HVAR_DATABASE              ERx("ii_database")
# define HVAR_DEBUG_SLEEP           ERx("ii_debug_sleep")
# define HVAR_BINARY_FILE_EXT       ERx("ii_binary_ext")
# define HVAR_ERROR_MSG             ERx("ii_error_message")
# define HVAR_ERROR_URL             ERx("ii_error_url")
# define HVAR_HTML_LOGFILE          ERx("ii_html_logfile")
# define HVAR_LOG_TYPE              ERx("ii_log_type")
# define HVAR_OUTPUT_DIR_LABEL      ERx("ii_output_dir")
# define HVAR_PAGE_HEADER           ERx("ii_page_header")
# define HVAR_PASSWORD              ERx("ii_password")
# define HVAR_PROCEDURE_NAME        ERx("ii_procedure")
# define HVAR_PROCVAR_COUNT         ERx("ii_procvar_count")
# define HVAR_QUERY_STATEMENT       ERx("ii_query_statement")
# define HVAR_REPORT_NAME           ERx("ii_report")
# define HVAR_REPORT_HEADER         ERx("ii_report_header")
# define HVAR_REPORT_LOCATION       ERx("ii_report_location")
# define HVAR_OUTPUT_DIR_NUM        ERx("ii_rwdir")
# define HVAR_SUCCESS_MSG           ERx("ii_success_message")
# define HVAR_SUCCESS_URL           ERx("ii_success_url")
# define HVAR_II_SYSTEM             ERx("ii_system")
# define HVAR_USERID                ERx("ii_userid")
# define HVAR_USERNUM               ERx("ii_usernum")
# define HVAR_VARNULL               ERx("ii_varnull")
# define HVAR_PROCVAR_TYPE          ERx("ii_procvar_type")
# define HVAR_PROCVAR_NAME          ERx("ii_procvar_name")
# define HVAR_PROCVAR_DATA          ERx("ii_procvar_data")
# define HVAR_PROCVAR_LEN           ERx("ii_procvar_length")
# define HVAR_RELATION_TYPE         ERx("ii_rel_type")
# define HVAR_HTTP_HOST             ERx("http_host")
# define HVAR_HTTP_API_EXT          ERx("http_api_ext")
# define HVAR_HTTP_AGENT            ERx("http_user_agent")
# define HVAR_HTTP_RMT_ADDR         ERx("http_remote_addr")
# define HVAR_HTTP_RMT_HOST         ERx("http_remote_host")

# define STR_RELATION_HTML          ERx("html")
# define STR_RELATION_XML           ERx("xml")
# define STR_RELATION_XMLPDATA      ERx("xmlpdata")
# define STR_RELATION_RAW           ERx("unformatted")

/* 2.5 */
# define HVAR_COOKIE                ERx("ii_cookie")
# define HVAR_ACTION                ERx("ii_action")
# define HVAR_PROFILE               ERx("ii_profile")
# define HVAR_AUTHTYPE              ERx("ii_authtype")
# define HVAR_ROWCOUNT              ERx("ii_rowcount")
# define HVAR_STATUS_NUMBER         ERx("ii_status_number")
# define HVAR_STATUS_TEXT           ERx("ii_status_text")
# define HVAR_STATUS_INFO           ERx("ii_status_info")
# define HVAR_UNIT                  ERx("ii_unit")
# define NULL_ERROR_STR             ERx("0")
# define CONNECT_STR                ERx("connect")
# define AUTO_DECLARE_STR           ERx("declare")
# define DOWNLOAD_STR               ERx("download")
# define DISCONNECT_STR             ERx("disconnect")
# define STR_BASIC_AUTH             ERx("Basic")
# define STR_DECL_SERVER            ERx("server")
# define STR_DECL_SESSION           ERx("session")
# define STR_DECL_PAGE              ERx("page")

# define NUM_OUT_DIR                5
# define NUM_MAX_PARM_STR           3
# define NUM_MAX_HVAR_LEN           258

#define WSS_UNIT_BEGIN              '['
#define WSS_UNIT_END                ']'

/* length of quote quote comma space */
# define REP_PARAM_DELIMITER_LEN    5

/* max number of report writer UTexe parameters */
# define MAX_RW_PARAM               6

/* type of location */
# define WCS_HTTP_LOCATION          (u_i4)2
# define WCS_II_LOCATION            (u_i4)4

/************** ACTIONS **************/
#define WCS_ACT_AVAILABLE           0
#define WCS_ACT_LOAD                1
#define WCS_ACT_DELETE              2
#define WCS_ACT_WAIT                3

/*************************************/
#define WCS_PUBLIC                  (i4)1
#define WCS_PRE_CACHE               (i4)2
#define WCS_PERMANENT_CACHE         (i4)4
#define WCS_SESSION_CACHE           (i4)8
#define WCS_EXTERNAL                (i4)16

/* Document type */
#define WSF_PAGE                    (i4)32
#define WSF_FACET                   (i4)64
#define WSF_TEMP                    (i4)0x8000

#define WSF_PAGE_STR                ERx("page")
#define WSF_FACET_STR               ERx("facet")
#define WSF_MAX_TYPE_STR            6

#define WCS_DEFAULT_UNIT            2
#define WCS_DEFAULT_DOCUMENT        50
#define WCS_DEFAULT_LOCATION        10
#define WSS_DEFAULT_APPLICATION     5

#define WCS_MEM_PERFORM_FILENAME(A, B, C) STprintf(A, "%d%d", B, C)
#define WCS_OBJ_LOCATION            0
#define WCS_OBJ_UNIT                1
#define WCS_OBJ_DOCUMENT            2
#define WCS_OBJ_UNIT_LOC            3
#define WSS_OBJ_PROFILE             4
#define WSS_OBJ_PROFILE_ROLE        5
#define WSS_OBJ_PROFILE_DB          6
#define WSS_OBJ_SYSTEM_VAR          7
#define WCS_OBJ_FILE                8
#define WSS_OBJ_APPLICATION         9

#define WSS_DEL_PROFILE_ROLE        0
#define WSS_DEL_PROFILE_DB          1
#define WSS_DEL_UNIT_LOC            2

#define QUERY_BLOCK_SIZE            256

#define HTML_ICE_TYPE_LINK          0
#define HTML_ICE_TYPE_OLIST         1
#define HTML_ICE_TYPE_ULIST         2
#define HTML_ICE_TYPE_COMBO         3
#define HTML_ICE_TYPE_TABLE         4
#define HTML_ICE_TYPE_IMAGE         5
#define HTML_ICE_TYPE_RAW           6
#define XML_ICE_TYPE_RAW            7
#define XML_ICE_TYPE_PDATA          8
#define XML_ICE_TYPE_XML            9
#define XML_ICE_TYPE_XML_PDATA      10

#define HTML_ICE_HTML_VAR           1
#define HTML_ICE_HTML_TEXT          2
#define HTML_ICE_HTML_BINARY        4

#define HTML_FIRST_NODE	            1
#define POST_FIRST_NODE             0

#define WPS_SESSION_NAME(A, B, C, D)  sprintf(name, "%d_%s[%s]", dbType, dbname, user)
#define WPS_TRANS_OPEN              1
#define WPS_TRANS_CLOSE             2
#define WPS_HTML_BLOCK              1
#define WPS_HTML_FOR_URL            2
#define WPS_URL_BLOCK               4
#define WPS_DOWNLOAD_BLOCK          8

#define WSM_SYSTEM                  1
#define WSM_USER                    2
#define WSM_ACTIVE                  4

#define HTML_ICE_INC_HTML           WSM_ACTIVE
#define HTML_ICE_INC_REPORT         6
#define HTML_ICE_INC_EXE            7
#define HTML_ICE_INC_MULTI          8

#define HTML_CONTENT                ERx("content-type: text/html\n")
#define HTTP_CONTENT                ERx("content-type: %s/%s\n")

#define STR_XML_VERSION             ERx("<?xml: version=\'1.0\' ?>")

#define BIN_CONTENT                 ERx("content-type: %s/%s\n")
#define STR_COOKIE                  ERx("Set-Cookie: %s%s%s; path=/ice-bin/%s.%s%s;")
#define STR_COOKIE_EXP              ERx("expires=%s;")
#define STR_COOKIE_MAXAGE           ERx("Max-Age=\"%s\";")

#define WSF_SYS_VAR_REP_DB          ERx("RepositoryDB")

#define WSF_SVR_APP                 ERx("application")
#define WSF_SVR_SESSGRP             ERx("session_grp")
#define WSF_SVR_UNIT                ERx("unit")
#define WSF_SVR_DOCUMENT            ERx("document")
#define WSF_SVR_DBUSER              ERx("dbuser")
#define WSF_SVR_ROLE                ERx("role")
#define WSF_SVR_USER                ERx("user")
#define WSF_SVR_DB                  ERx("database")
#define WSF_SVR_UR                  ERx("user_role")
#define WSF_SVR_UD                  ERx("user_database")
#define WSF_SVR_DOCROLE             ERx("document_role")
#define WSF_SVR_DOCUSER             ERx("document_user")
#define WSF_SVR_UNITROLE            ERx("unit_role")
#define WSF_SVR_UNITUSER            ERx("unit_user")
#define WSF_SVR_UNITLOC             ERx("unit_location")
#define WSF_SVR_UNITCOPY            ERx("unit_copy")

#define WSF_SVR_ACT_SESS            ERx("active_users")
#define WSF_SVR_USR_SESS            ERx("ice_users")
#define WSF_SVR_USR_TRANS           ERx("ice_user_transactions")
#define WSF_SVR_USR_CURS            ERx("ice_user_cursors")
#define WSF_SVR_LOC                 ERx("ice_locations")
#define WSF_SVR_CACHE               ERx("ice_cache")
#define WSF_SVR_CONN                ERx("ice_connect_info")

#define WSF_SVR_PROFILE             ERx("profile")
#define WSF_SVR_PR                  ERx("profile_role")
#define WSF_SVR_PD                  ERx("profile_database")
#define WSF_SVR_TAG_2_STR           ERx("TagToString")
#define WSF_SVR_GET_LOC             ERx("dir")
#define WSF_SVR_GET_VAR             ERx("getVariables")
#define WSF_SVR_GET_SVRVAR          ERx("getSvrVariables")
#define WSF_SVR_ICE_VAR             ERx("ice_var")

#define WSF_SVR_ACT_INS             ERx("insert")
#define WSF_SVR_ACT_UPD             ERx("update")
#define WSF_SVR_ACT_DEL             ERx("delete")
#define WSF_SVR_ACT_SEL             ERx("select")
#define WSF_SVR_ACT_RET             ERx("retrieve")
#define WSF_SVR_ACT_IN              ERx("in")
#define WSF_SVR_ACT_OUT             ERx("out")

#define WSS_ACTION_TYPE_INS         1
#define WSS_ACTION_TYPE_UPD         2
#define WSS_ACTION_TYPE_DEL         3
#define WSS_ACTION_TYPE_SEL         4
#define WSS_ACTION_TYPE_RET         5

#define WSF_USR_ADM                 (i4)1
#define WSF_USR_SEC                 (i4)2
#define WSF_USR_UNIT                (i4)4
#define WSF_USR_MONIT               (i4)8

#define WSF_DLL_LOAD_FCT            ERx("InitICEServerExtension")
#define WSF_UNDEFINED               ERx("undefined")
#define WSF_CHECKED                 ERx("checked")
#define WSF_COLUMN_NAME             ERx("ColumnName")

#define WSF_STR_RESOURCE            32

/* if macro */
#define WPS_TRUE                    1
#define WPS_FALSE                   2
#define WPS_EQUAL                   3
#define WPS_NOT_EQUAL               4
#define WPS_GREATER                 5
#define WPS_LOWER                   6
#define WPS_AND                     7
#define WPS_OR                      8
#define WPS_OPEN_PARENTH            10
#define WPS_CLOSE_PARENTH           11
#define WPS_TEXT                    12

/* active session types */
#define WSM_NORMAL                  0
#define WSM_CONNECT                 1
#define WSM_DISCONNECT              2
#define WSM_AUTO_DECL               3
#define WSM_DOWNLOAD                4

/* ICE Right */
#define WSF_READ_RIGHT              (i4)1
#define WSF_INS_RIGHT               (i4)2
#define WSF_UPD_RIGHT               (i4)4
#define WSF_DEL_RIGHT               (i4)8
#define WSF_EXE_RIGHT               (i4)16

#define COPY_BU_STRING              ERx("ICE (Ingres v1): Business Unit Copy")

#define	WSF_READING_BLOCK           4096
#define MULTIPART                   ERx("multipart")
#define NO_UPLOAD                   0
#define UPLOAD                      1
#define	UPLOAD_FILE                 2

#define BOUNDARY                    ERx("boundary=")
#define DISPOSITION                 ERx("Content-Disposition: form-data")
#define STR_LENGTH_SIZE             3

#define	CONNECTED                   1
#define	HTML_API                    2
#define	C_API                       4

#define DEFAULT_FILE_CAT            ERx("application")
#define DEFAULT_FILE_EXT            ERx("octet-stream")

#endif
