/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: icemain.h
**
** Description:
**      defines, structs and function declarations for ICE 2.0
**      This is the main header for ICE implementation files
**
** History:
**      20-nov-1996 (wilan06)
**          Created
**      28-jan-1997 (harpa06)
**          Added server variable definitions for NSAPI and ADI.
**      06-feb-1997 (harpa06)
**          Added STR_ICE_SUCCESS_MSG_FILTER, STR_ICE_ERROR_MSG_FILTER and
**          STR_ICE_WARNING_MSG_FILTER.
**          
**          Removed references to an ICE log file in ICECONFIGDATA structure 
**          and added fLogType and ICELOGTYPE.
**
**          Added platform-specfic #defines and STR_INGRES_ICE
**      14-feb-1997 (harpa06)
**          Added fAllowApp to tagICECONFIGDATA.
**          Added STR_ICE_CLAPP_PREFIX, STR_INGRES_ICE, STR_PERIOD,
**          STR_ICE_CLAPP_PREFIX, APIVAR_CONTENT_TYPE. 
**
**          Modified STR_ON and STR_OFF so they're the same in the config.dat
**          file as other resources.
**
**          Added fAllowApp to ICECONFIGDATA structure.
**
**          Added STR_DEFAULT
**      07-mar-1997 (harpa06)
**          Added ICE connection manager mutex functions so they can be used by
**          the logging system.
**
**          Removed STR_INGRES_ICE and changed log file message filters since
**          all ICE log entries are now written to one location.
**
**          Moved config.dat resources here from icepriv.h since they are now
**          used  by the install program as well.
**      31-mar-1997 (harpa06)
**          Added HasTags() prototype and other config.dat definitions.
**      10-apr-1997 (harpa06)
**          Modified ICEHTMLFORMATTER structure to include a head and table
**          cell attributes.
**
**          Added a few HTTP status codes.
**      21-apr-1997 (harpa06)
**          Changed ii_binary_file_ext to ii_binary_ext. The previous is too
**          long.
**
**          Removed the ".so" from the UNIX library extensions so the UNIX
**          libraries can be found. (Bug #81764)
**      28-may-1997 (harpa06)
**          Added support for overriding the default database name for 
**          macro processor.
**      25-jun-1997 (harpa06)
**          Added CHAR_SINGLEQUOTE.
**      28-jul-1997 (harpa06)
**          Removed "Token" from ICEUSER structure. It isn't used.
**      19-aug-1997 (harpa06)
**          Fixed a few typos.
**      17-Oct-97 (fanra01)
**          Add new keywords.
**          TEMPLATE    used for specifying result is to be used as a template.
**          USERID      specify a user with the query.
**          PASSWORD    specify a password for the user.
**          VAR         so that values may be used on the html page.
**          EXT         specifies the extension to be given to a blob file
**                      when extracted.  Applies to all blobs in a query.
**          Add parameter to force a password to be entered if a userid is
**          specified.
**          Add a global wrapper for ReplaceMacroTags.
**          Made SubstituteQueryParms global.
**      21-oct-1997 (harpa06)
**          Added ADIVAR_AUTHORIZATION to obtain a user's userid and password
**          from a Spyglass 1.2 (butler) server.
**      28-oct-1997 (harpa06)
**          Added NSAPIVAR_AUTHORIZATION to obtain a user's userid and password
**          from a Netscape Enterprise 2.x server.
**
**          Removed APIVAR_AUTH_TYPE since it is not used by any server.
**      12-dec-1997 (harpa06)
**          Removed szPassword from the tagICECONFIGDATA structure since we 
**          do not use a default password for the default userid. The default 
**          userid is already a trusted id.
**      09-Jan-98 (fanra01)
**          Add the NULLVAR keyword to enable parameters to be replaced with
**          a defined string.
**      15-jan-98 (harpa06)
**          Added support for user-defined table column headers. s88236
**          Added support for obtaining DBMS error messages. s88270
**      20-jan-98 (harpa06)
**          Added ReplaceNewlines() prototype.
**      28-jan-98 (harpa06)
**          Added fHaveMacro to ICEPAGE structure.
**          Added szMacroDoc to ICEREQUEST structure.
**      20-Apr-98 (fanra01)
**          Add ICEERget prototype and modified mutex prototypes to include
**          recursion counts and thread owners.
**	15-Oct-98 (i4jo01)
**	    Added define for nullset report flag.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-sep-2000 (mcgem01)
**	    replace nat and longnat with i4.
*/

# ifndef ICEMAIN_H_INCLUDED
# define ICEMAIN_H_INCLUDED

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <lo.h>
# include <er.h>

# include <idb.h>

/* Platform-dependent commands and binaries */
# ifdef NT_GENERIC
# define COPY_CMD           "copy"
# define INSTALL_EXE        "iceinst.exe"
# define CGI_LIB            "ice.exe"
# define SPYGLASS_LIB       "icesgadi.dll"
# define NETSCAPE_LIB       "icensapi.dll"
# define ISAPI_LIB          "iceisapi.dll"
# endif /* NT_GENERIC */

# ifdef UNIX
# define COPY_CMD           "cp"
# define INSTALL_EXE        "iceinst"
# define CGI_LIB            "ice"
# define SPYGLASS_LIB       "icesgadi.1"
# define NETSCAPE_LIB       "icensapi.1"
# define ISAPI_LIB          "iceisapi.1"
# endif /* UNIX */

# ifdef VMS
# define COPY_CMD "copy/prot=(s:rwed,o:rwed,g:rwed,w:we)"
# define INSTALL_EXE        "iceinst"
# define CGI_LIB            "ice"
# define SPYGLASS_LIB
# define NETSCAPE_LIB
# define ISAPI_LIB
# define WEBSCRIPT "ice.com"
# endif /* VMS */

# if !( defined( COPY_CMD ) && defined( CGI_LIB ) && defined (INSTALL_EXE) && \
  defined (SPYGLASS_LIB) && defined (NETSCAPE_LIB) && defined (ISAPI_LIB))
#  error Platform-dependent macros required for compilation are not defined.
# endif


/* Misc constants */
# define NUM_OUT_DIR                    5
# define NUM_MAX_PARM_STR               3
# define NUM_MAX_HVAR_LEN               258
# define NUM_MAX_TIME_STR               25

# define STR_NONE                       ERx("<none>")
# define STR_DEFAULT                    ERx (" (default)")
# define STR_PARAM_SEPARATOR            ERx(", ")
# define STR_EQUALS_SIGN                ERx("=")
# define STR_MINUS_SIGN                 ERx("-")
# define STR_COLON                      ERx(":")
# define STR_EMPTY                      ERx("")
# define STR_OBRACE                     ERx("[")
# define STR_CBRACE                     ERx("]")
# define STR_SPACE                      ERx(" ")
# define STR_NEWLINE                    ERx("\n")
# define STR_CRLF                       ERx("\r\n")
# define STR_COMMA                      ERx(",")
# define STR_QUESTION_MARK              ERx("?")
# define STR_SEMICOLON                  ERx(";")
# define STR_PERIOD			ERx(".")
# define STR_NULL                       ERx("NULL")

/* macro processor defines */
# define STR_ICE_TAG                    ERx("#ICE")
# define STR_ICE_KEYWORD_TYPE           ERx("TYPE")
# define STR_ICE_KEYWORD_SQL            ERx("SQL")
# define STR_ICE_KEYWORD_DATABASE       ERx("DATABASE")
# define STR_ICE_KEYWORD_ATTR           ERx("ATTR")
# define STR_ICE_KEYWORD_LINKS          ERx("LINKS")
# define STR_ICE_KEYWORD_TYPELINK       ERx("LINK")
# define STR_ICE_KEYWORD_TYPEOLIST      ERx("OLIST")
# define STR_ICE_KEYWORD_TYPEULIST      ERx("ULIST")
# define STR_ICE_KEYWORD_TYPETABLE      ERx("TABLE")
# define STR_ICE_KEYWORD_TYPECOMBO      ERx("SELECTOR")
# define STR_ICE_KEYWORD_TYPEIMAGE      ERx("PLAIN")
# define STR_ICE_KEYWORD_TYPERAW        ERx("UNFORMATTED")
# define STR_ICE_KEYWORD_TYPETEMPLATE   ERx("TEMPLATE")
# define STR_ICE_KEYWORD_USERID         ERx("USERID")
# define STR_ICE_KEYWORD_PASSWD         ERx("PASSWORD")
# define STR_ICE_KEYWORD_INSERTVAR      ERx("VAR")
# define STR_ICE_KEYWORD_EXTENSION      ERx("EXT")
# define STR_ICE_KEYWORD_HEADERS        ERx("HEADERS")
# define STR_ICE_KEYWORD_VARNULL        ERx("NULLVAR")

# define STR_ICE_BINARY                 ERx("binary data")

# define STR_SELECT_TID                 ERx("tid, ")
# define HASH_TABLE_SIZE                128
# define LEN_HEX_URL_CHAR               2
# define ICE_MAX_QUERY_STRING           0x7FFF
# define STR_ON                         ERx("ON")
# define STR_OFF                        ERx("OFF")
# define STR_YES                        ERx("Yes")
# define STR_NO                         ERx("No")
# define STR_ONE                        ERx("1")
# define STR_ZERO                       ERx("0")
# define DEFAULT_DEBUG_SLEEP_TIME       30

# ifdef VMS
#  define STR_CMDLINE_QUOTE     ERx("'")
#  define CHAR_PATH_SEPARATOR   '.'
# else
#  define STR_CMDLINE_QUOTE     ERx("'")
#  ifdef NT_GENERIC
#   define CHAR_PATH_SEPARATOR  '\\'
#  else
#   define CHAR_PATH_SEPARATOR  '/'
#  endif
# endif

# define CHAR_URL_SEPARATOR     '/'
# define CHAR_SINGLEQUOTE       '\''

# define STR_FILE_SUFFIX_SEPARATOR ERx(".")
# define STR_QUOTE              ERx("\"")
# define STR_BACKQUOTE          ERx("`")
# define STR_SINGLEQUOTE        ERx("'")

# define NUM_MIN_RW_PARAM       1
# define STR_GATEWAY_SEPARATOR  ERx("/")
# define STR_ICE_REPORT_PREFIX  ERx("ir")
# define STR_ICE_MACRO_PREFIX   ERx("im")
# define STR_ICE_CLAPP_PREFIX   ERx("ica")
# define STR_RW_DATABASE_PARAM  ERx("database = %S, ")
# define STR_RW_USER_PARAM      ERx("user = %S, ")
# define STR_RW_FLAGS_PARAM     ERx("flags = %S, ")
# define STR_RW_FILE_PARAM      ERx("file = %S, ")
# define STR_RW_SRCFILE_PARAM   ERx("sourcefile = %S, ")
# define STR_RW_REPNAME_PARAM   ERx("report = %S, ")
# define STR_RW_MISC_PARAM      ERx("param = %S, ")

# define STR_ING_RW_FLAGS       ERx("-s +#:_=")

# define STR_ING_RW_FLAGS_NS    ERx("-s +#:_= -h")

# define STR_ING_RW_UTEXE_NAME  ERx("report")

# define STR_LOGFILE_EXT        ERx("log")
# define STR_HTML_MIME_TYPE     ERx("text/html")
# define STR_HTML_EXT           ERx("html")

# define STR_UNKNOWN_USER       ERx("(unknown user)")
# define STR_UNKNOWN_ADDR       ERx("(unknown client address)")
# define STR_UNKNOWN_REPORT     ERx("Unknown report")
# define STR_HTTP_PREFIX        ERx("http://")
# define STR_DEF_BINARY_PREFIX  ERx("ice")

# define STR_ICE_LOG_FILENAME   ERx("ice.log")
# define STR_ICE_LOG_SUBDIR     ERx("files")

/* Log file message filters */
# define STR_ICE_SUCCESS_MSG_FILTER     ERx ("Success:")
# define STR_ICE_ERROR_MSG_FILTER       ERx ("Error:")
# define STR_ICE_WARNING_MSG_FILTER     ERx ("Warning:")

/* Config.dat parameters. Note that these require that the host name
** is substituted (using sprintf) for %s. %d is number ranging from 1 to
** infinity.
*/
# define CONF_CONFIG_WS         ERx("ii.%s.ice.config_ws")
# define CONF_CONFIG_WS_DIR     ERx("ii.%s.ice.config_ws_dir")
# define CONF_HTML_HOME         ERx("ii.%s.ice.html_home")
# define CONF_BIN_DIR           ERx("ii.%s.ice.cgi-bin_dir")
# define CONF_BIN_REF           ERx("ii.%s.ice.cgi-bin_ref")
# define CONF_ALLOW_DYN_SQL     ERx("ii.%s.ice.security.allow_dsqlq")
# define CONF_ALLOW_EXE_APP     ERx("ii.%s.ice.security.allow_exeapp")
# define CONF_ALLOW_DB_OVR      ERx("ii.%s.ice.security.allow_dbovr")
# define CONF_FORCE_PASSWORD    ERx("ii.%s.ice.security.force_passwd")
# define CONF_LOG_TYPE          ERx("ii.%s.ice.log.type")
# define CONF_LOG_VIEW          ERx("ii.%s.ice.log.view")
# define CONF_OUTPUT_DIR1_LOC   ERx("ii.%s.ice.rw_dir1_location")
# define CONF_OUTPUT_DIR1_TIME  ERx("ii.%s.ice.rw_dir1_time")
# define CONF_OUTPUT_DIR2_LOC   ERx("ii.%s.ice.rw_dir2_location")
# define CONF_OUTPUT_DIR2_TIME  ERx("ii.%s.ice.rw_dir2_time")
# define CONF_OUTPUT_DIR3_LOC   ERx("ii.%s.ice.rw_dir3_location")
# define CONF_OUTPUT_DIR3_TIME  ERx("ii.%s.ice.rw_dir3_time")
# define CONF_OUTPUT_DIR4_LOC   ERx("ii.%s.ice.rw_dir4_location")
# define CONF_OUTPUT_DIR4_TIME  ERx("ii.%s.ice.rw_dir4_time")
# define CONF_OUTPUT_DIR5_LOC   ERx("ii.%s.ice.rw_dir5_location")
# define CONF_OUTPUT_DIR5_TIME  ERx("ii.%s.ice.rw_dir5_time")
# define CONF_SERVER_NAME       ERx("ii.%s.ice.server_name")
# define CONF_DEFAULT_USERID    ERx("ii.%s.ice.default_userid")
# define CONF_DEFAULT_PASSWORD  ERx("ii.%s.ice.default_password")
# define CONF_DEFAULT_DATABASE  ERx("ii.%s.ice.default_database")
# define CONF_CONN_TIMEOUT      ERx("ii.%s.ice.db_conn_timeout")
# define CONF_OUTPUT_LABEL      ERx("ii.%s.ice.dir%d_label")
# define CONF_OUTPUT_LOC        ERx("ii.%s.ice.dir%d_location")
# define CONF_OUTPUT_TIME       ERx("ii.%s.ice.dir%d_time")
# define CONF_OUTPUT_DEFAULT    ERx("ii.%s.ice.dir_default_location")
# define CONF_LIB_STATUS        ERx("ii.%s.ice.component.status.lib")
# define CONF_DSQL_TUT_STAT     ERx("ii.%s.ice.component.status.tut.dsql")
# define CONF_MCRO_TUT_STAT     ERx("ii.%s.ice.component.status.tut.mcro")
# define CONF_MCRO_DSK_STAT     ERx("ii.%s.ice.component.status.dsk.mcro")
# define CONF_DSQL_DSK_STAT     ERx("ii.%s.ice.component.status.dsk.dsql")
# define CONF_TUTORIAL_DB       ERx("ii.%s.ice.component.tutorial.db")
# define CONF_ROOT_TUT_DIR      ERx("ii.%s.ice.component.tutorial.dir")
# define CONF_MCRO_TUT_DIR      ERx("ii.%s.ice.component.tutorial.dir.mcro")
# define CONF_DSQL_TUT_DIR      ERx("ii.%s.ice.component.tutorial.dir.dsql")

/* HTML variable names used by ICE. New system variable names
** are added, you should also add them to the array systemHTMLVars
** in the file icevproc.c
*/
# define HVAR_APPLICATION       ERx("ii_application")
# define HVAR_DATABASE          ERx("ii_database")
# define HVAR_DEBUG_SLEEP       ERx("ii_debug_sleep")
# define HVAR_BINARY_FILE_EXT   ERx("ii_binary_ext")
# define HVAR_ERROR_MSG         ERx("ii_error_message")
# define HVAR_ERROR_URL         ERx("ii_error_url")
# define HVAR_HTML_LOGFILE      ERx("ii_html_logfile")
# define HVAR_LOG_TYPE          ERx("ii_log_type")
# define HVAR_OUTPUT_DIR_LABEL  ERx("ii_output_dir")
# define HVAR_PAGE_HEADER       ERx("ii_page_header")
# define HVAR_PASSWORD          ERx("ii_password")
# define HVAR_PROCEDURE_NAME    ERx("ii_procedure")
# define HVAR_PROCVAR_COUNT     ERx("ii_procvar_count")
# define HVAR_QUERY_STATEMENT   ERx("ii_query_statement")
# define HVAR_REPORT_NAME       ERx("ii_report")
# define HVAR_REPORT_HEADER     ERx("ii_report_header")
# define HVAR_REPORT_LOCATION   ERx("ii_report_location")
# define HVAR_REPORT_NULLSET    ERx("ii_report_nullset")
# define HVAR_OUTPUT_DIR_NUM    ERx("ii_rwdir")
# define HVAR_SUCCESS_MSG       ERx("ii_success_message")
# define HVAR_SUCCESS_URL       ERx("ii_success_url")
# define HVAR_II_SYSTEM         ERx("ii_system")
# define HVAR_USERID            ERx("ii_userid")
# define HVAR_VARNULL           ERx("ii_varnull")

# define HVAR_PROCVAR_TYPE      ERx("ii_procvar_type")
# define HVAR_PROCVAR_NAME      ERx("ii_procvar_name")
# define HVAR_PROCVAR_DATA      ERx("ii_procvar_data")
# define HVAR_PROCVAR_LEN       ERx("ii_procvar_length")

/* CGI Variable names */
# define VAR_PATH_INFO          ERx("PATH_INFO")
# define VAR_CONTENT_LENGTH     ERx("CONTENT_LENGTH")
# define VAR_QUERY_STRING       ERx("QUERY_STRING")
# define VAR_REQUEST_METHOD     ERx("REQUEST_METHOD")
# define VAR_HTTP_AUTH          ERx("HTTP_AUTHORIZATION")
# define VAR_PATH_TRANSLATED    ERx("PATH_TRANSLATED")
# define VAR_REMOTE_ADDR        ERx("REMOTE_ADDR")
# define VAR_REMOTE_HOST        ERx("REMOTE_HOST")

/* Common ADI and NSAPI server variable names */
# define APIVAR_PATH_INFO       ERx("path-info")
# define APIVAR_CONTENT_LENGTH  ERx("content-length")
# define APIVAR_CONTENT_TYPE    ERx("content-type")

/* NSAPI server variable names */
# define NSAPIVAR_AUTHORIZATION ERx("authorization")

/* ADI server variable names */
# define ADIVAR_AUTHORIZATION   ERx("Authorization")

# define STR_POST_METHOD        ERx("POST")
# define STR_GET_METHOD         ERx("GET")
# define STR_BASIC_AUTH         ERx("Basic ")

/* HTTP status codes */
# define HTTP_STATUS_OK         200
# define HTTP_STATUS_NOTAUTH    401
# define HTTP_STATUS_FORBIDDEN  403

/* macro processor defines */
# define STR_ICE_TAG            ERx("#ICE")
# define STR_ICE_KEYWORD_TYPE   ERx("TYPE")
# define STR_ICE_KEYWORD_SQL    ERx("SQL")

/* initialization and termination types */
# define ICE_INIT_LOAD           1
# define ICE_INIT_SESSION        2
# define ICE_TERM_SESSION        4
# define ICE_TERM_UNLOAD         8

/* returns the number of items in an array */
# define ARRAYITEMCOUNT(a) (sizeof(a)/sizeof(*a))

/*
** Identifies the type of message to be logged and displayed on the client
** browser
*/
typedef enum icemsgtype
{
    ICE_SUCCESS, ICE_ERROR, ICE_WARNING, ICE_MESSAGE
} ICEMSGTYPE;

/*
** Different rules determing when to add a log entry into the server's log file.
*/
typedef enum icelogtype
{
    LOG_ERROR, LOG_SUCCESS, LOG_SUCCESS_AND_WARNING, LOG_SUCCESS_AND_ERROR,
    LOG_WARNING, LOG_WARNING_AND_ERROR, LOG_ALL, LOG_NONE
} ICELOGTYPE;

/* HTTP request methods */
typedef enum reqmethod
{
    REQUEST_METHOD_ERROR, REQUEST_METHOD_POST, REQUEST_METHOD_GET
} REQMETHOD;

/* Output types for Macro HTML */
typedef enum iceouttype
{
    ICE_OUTTYPE_TABLE, ICE_OUTTYPE_OLIST, ICE_OUTTYPE_ULIST, ICE_OUTTYPE_COMBO,
    ICE_OUTTYPE_LINK, ICE_OUTTYPE_IMAGE, ICE_OUTTYPE_RAW, ICE_OUTTYPE_TEMPLATE
} ICEOUTTYPE;

/* holds a single ICE2.0 output directory */
typedef struct tagICEOUTPUTDIR
{
    char        szOutDir[MAX_LOC+1];
    char        szOutLabel[MAX_LOC+1];
    long        dirTimeout;
} ICEOUTPUTDIR, *PICEOUTPUTDIR;
   
/* Holds the configuration information retrieved from config.dat */
typedef struct tagICECONFIGDATA
{
    char           szInstDir[MAX_LOC+1];               /* II_SYSTEM */
    char           szHTMLHome[MAX_LOC+1];              /* root HTML dir */
    char           szBinDir[MAX_LOC+1];                /* CGI bin dir */
    int            fAllowSQL;                          /* dyn SQL allowed */
    int            fAllowApp;               	       /* client apps allowed */
    int            fAllowDBOvr;                        /* override dbname for
                                                          macro processor */
    int            fForcePasswd;                       /* force use of passwd*/
    ICELOGTYPE     fLogType;                           /* log type */
    char           szDefaultDB[DB_MAXNAME+1];          /* default ice target */
    char           aszOutDir[NUM_OUT_DIR][MAX_LOC+1];  /* output subdirs */
    int            aDirTime[NUM_OUT_DIR+1];            /* file timeouts */
    char           szWebServer[MAX_LOC+1];             /* web server */
    char           szUser[DB_MAXNAME+1];               /* default user id */
    int            timeout;                            /* default conn
                                                          timeout */
    char*          pszDefaultExt;                      /* generated file ext */
    char*          pszMIMEType;                        /* MIME type for
                                                          responses */
    char*          pszReportTool;                      /* reporting tool exe */
    int            cOutDir;                            /* count of v2 dirs */
    PICEOUTPUTDIR  aodOutDir;                          /* array of v2 dirs */
    char*          pszDefaultOutLabel;                 /* output dir label */
} ICECONFIGDATA, *PICECONFIGDATA;

GLOBALDEF ICECONFIGDATA   icd;

/* Information that identifies a user */
typedef struct tagICEUSER
{
    char*       Userid;
    char*       Password;
} ICEUSER, *PICEUSER;

/* Holds an HTML page under construction */
typedef struct tagICEPAGE
{
    char*       page;           /* the data block */
    int         pageLen;        /* the length of the data */
    int         pageSize;       /* the size of the data block */
    int         blockSize;      /* how much we grow the block by next time */
    ICERES      fHaveMacro;     /* did we make an attempt to process a macro */
    ICERES      fSuccess;       /* true if client request succeeds */
    ICERES      fWarning;       /* true if there are warning messages */
} ICEPAGE, *PICEPAGE;

/* Holds a user's request */
typedef struct tagICEREQUEST
{
    char*       pszRequest;     /* ptr to request text */
    char*       pszValueBlock;  /* ptr to memory holding non-request values */
    char        szMacroDoc [MAX_LOC + 1]; /* path to document to process */
    PTR         hashTabSys;     /* System var hash table context ptr */
    PTR         hashTabUsr;     /* Usr var hash table context ptr */
} ICEREQUEST, *PICEREQUEST;

/* Holds the information about a report to be executed */
typedef struct tagICEREPORTINFO
{
    char*       pszName;
    char*       pszLocation;
    char*       pszDB;
    bool        fGateway;
    char*       pszUser;
    char*       pszPass;
    char*       pszParams;
    LOCATION    loOut;
    char*	pszNullSet;
} ICEREPORTINFO, *PICEREPORTINFO;

/* temporary file used to hold blob data */
typedef struct tagICETEMPFILE
{
    char        szFile [MAX_LOC+1];
    LOCATION    loFile;
    char        szPrefix [LO_FPREFIX_MAX+1];
    char        szExt [LO_FSUFFIX_MAX+2];
    FILE*       fp;
} ICETEMPFILE, *PICETEMPFILE;

# include <iia.h>

/* generated link */
typedef struct tagICELINK
{
    char*       pszURL;
    char*       pszCol;
} ICELINK, *PICELINK;

/* user-defined column headers */
typedef struct tagICECHDR
{
    char*       pszTCol;
    char*       pszUCol;
} ICECHDR, *PICECHDR;

/* functions to format the output, plus the output option keyword values */
typedef struct tagICEHTMLFORMATTER
{
    ICEOUTTYPE  type;
    ICEPFN      OutputStart;
    ICEPFN      OutputEnd;
    ICEPFN      RowStart;
    ICEPFN      RowEnd;
    ICEPFN      TextItem;
    ICEPFN      BinaryItem;
    char*       pszSuccess;             /* Success msg for non-SELECT stmts*/
    char*       pszError;               /* Invalid stmt msg */
    char*       pszHeadAttr;            /* HTML head attributes */
    char*       pszAttr;                /* HTML body attributes */
    char*       pszCellAttr;            /* HTML TABLE cell attributes */
    char*       pszLinks;               /* unparsed links */
    char*       pszCHdrs;               /* unparsed column headers */
    PICELINK    aLink;                  /* parsed link array */
    PICECHDR    aCHdr;                  /* parsed column header array */
} ICEHTMLFORMATTER, *PICEHTMLFORMATTER;

/* UG predecs */
STATUS IIUGhiHtabInit(i4, void (*allocfail)(), i4(*comp)(), i4  (*hfunc)(),
                      PTR* tab);
STATUS IIUGhfHtabFind(PTR tab, char *key, PTR *data);
STATUS IIUGheHtabEnter(PTR tab, char *s, PTR data);
VOID IIUGhrHtabRelease(PTR tab);
STATUS IIUGhdHtabDel(PTR tab, char *key);
i4  IIUGhsHtabScan(PTR tab, bool cont, char** key, PTR* dat);

/* CM predecs */
u_i2 * CMgetAttrTab (void);
char *CMgetCaseTab  (void);

ICERES ICEMain (PICECLIENT pic);
ICERES ICEInit (PICECLIENT pic, II_UINT4 type);
ICERES ICETerm (PICECLIENT pic, II_UINT4 type);
ICERES ICELogMessage (PICECLIENT pic, char* message, ICEMSGTYPE);
ICERES ICEGetUserAuthorization (PICECLIENT pic, PICEREQUEST pir);

/* Connection manager */
STATUS ICMInit (i4 timeout);
STATUS ICMTerminate (void);
STATUS ICMGetConnection (char* pszDb, PICEUSER piu, II_PTR* phconn, 
                         char **ppszMessage);
STATUS ICMDropConnection (char *dbName, II_PTR hconn, char **ppszMessage);

/* HTML Page functions */
ICERES ICEPageCreate (PICECLIENT pic, PICEREQUEST pir);
ICERES ICEPageAppend (PICECLIENT pic, char* msg);
ICERES ICEPageEscapeAndAppend (PICECLIENT pic, char* msg);
ICERES ICEPagePrepend (PICECLIENT pic, char* msg, int len);
ICERES ICEPageSubmit (PICECLIENT pic, PICEREQUEST pir);
ICERES ICEPageSubmitMacro (PICECLIENT pic, PICEREQUEST pir);
ICERES ICENewHTMLFormatter (ICEOUTTYPE outType, PICEHTMLFORMATTER* ppihf);
ICERES ICEFreeHTMLFormatter (PICEHTMLFORMATTER pihf);
ICERES ICEInitColumnLinks (PICEHTMLFORMATTER pihf, IIAPI_GETDESCRPARM* pgdp);
ICERES ICEInitColumnHeaders (PICEHTMLFORMATTER pihf, IIAPI_GETDESCRPARM* pgdp);


ICERES ICEProcessMacroHTML (PICECLIENT pic);
ICERES ICEWriteLog (PICECLIENT pic, char* message);

/* Utility functions */
ICERES ICEGetOutputDirLoc (PICECLIENT pic, PICEREQUEST pir, LOCATION* ploOut,
                           char* pszPathBuf);
ICERES ICECreateTempLocation (LOCATION* plo, char* prefix, char* ext,
                                   char* buffer);

ICERES ICEReplaceMacroTags (PICECLIENT pic, PICEREQUEST pir, FILE* fpR,
                            char* pszBuf, i4 cbBuf);

ICERES HasTags (char* pszBuf);

VOID ICETrace (i4 nDir, char* pszFmt, ...);

void ICEFormatURL (char* pszWebServer, char* pszRelPath, char* pszFile,
                   char* pszExtra,
                   char** ppszURL);
void ReplaceNewlines (char** pszMessage);
void Decode64 (char* pszIn, char* szOut);
char* AllocateIOBuffer (LOCATION* plo, i4 bufMax, i4* pcbFile, i4* pcbBuffer);
void MemMove (char* source, char* dest, int cb);
void URLEscapeChar (char* pszIn, char* pszOut, int* pcbAppended);
char* GetToken (char** ppszSrc, char* pszSeps, int* ptokLen);
char* GetTokenUnquoted (char** ppszSrc, char* pszSeps, char* pszQuote,
                       int* ptokLen);
void DoDebugSleep (PICEREQUEST pir);
bool ICEWalkHeap (void);
char* SubstituteQueryParms (char* pszQuery, PICEREQUEST pir, int* pcParm);
STATUS ICEERget (ER_MSGID id, PTR* ppszMsg);

/* Mutex utility functions */
#include <mu.h>
void MutexCreate (MU_SEMAPHORE *mutex);
void
MutexRelease (MU_SEMAPHORE *mutex, i4* pnCount, CS_THREAD_ID* pThreadOwner);
void
MutexRequest (MU_SEMAPHORE *mutex, i4* pnCount, CS_THREAD_ID* pThreadOwner);
void MutexDestroy (MU_SEMAPHORE *mutex);
# endif
