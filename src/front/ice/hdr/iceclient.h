/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      02-Nov-1998 (fanra01)
**          Add rmt_addr and rmt_host
**      13-Nov-98 (fanra01)
**          Define strings for Netscape.
**      05-Jan-1999 (fanra01)
**          Add api status to indicate value to return.
**      11-Mar-1999 (peeje01)
**          Conditinonally set shared library object extenstions
**	09-aug-1999 (mcgem01)
**	    Changed nat and longnat to i4.
**	29-Jul-1999 (muhpa01)
**	    Set API_TYPE, "1.sl", for HP and CGI_TYPE as "cgi"
**	    generically for UNIX.
**      20-Jan-2000 (fanra01)
**          Sir 100122
**          Add Spyglass and Apache names for HTTP header elements.
**      03-May-2000 (fanra01)
**          Bug 100312
**          Add variables for obtaining SSL information from the various
**          interfaces.
**      06-Jun-2000 (fanra01)
**          Bug 101345
**          Add storage for the target node name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2001 (fanra01)
**          Bug 104362
**          Changed the case of the Apache API content-type string for
**          obtaining content-type from the request headers.
**      02-Jul-2001 (fanra01)
**          Bug 104296
**          Add method as a Netscape header type.
**          Bug 104297
**          Add Location as a Netscape header type.
*/
#ifndef ICECLI_INCLUDED
#define ICECLI_INCLUDED

#include <ddfcom.h>

#define     HTML_API            2
#define     C_API               4

/* CGI Variable names */
# define    VAR_PATH_INFO       ERx("PATH_INFO")
# define    VAR_CONTENT_LENGTH  ERx("CONTENT_LENGTH")
# define    VAR_CONTENT_TYPE    ERx("CONTENT_TYPE")
# define    VAR_QUERY_STRING    ERx("QUERY_STRING")
# define    VAR_REQUEST_METHOD  ERx("REQUEST_METHOD")
# define    VAR_HTTP_AUTH       ERx("HTTP_AUTHORIZATION")
# define    VAR_PATH_TRANSLATED ERx("PATH_TRANSLATED")
# define    VAR_REMOTE_ADDR     ERx("REMOTE_ADDR")
# define    VAR_REMOTE_HOST     ERx("REMOTE_HOST")
# define    VAR_HTTP_HOST       ERx("HTTP_HOST")
# define    VAR_HTTP_COOKIE     ERx("HTTP_COOKIE")
# define    VAR_SERVER_PORT     ERx("SERVER_PORT")
# define    VAR_HTTP_AGENT      ERx("HTTP_USER_AGENT")
# define    VAR_HTTP_RMT_ADDR   ERx("REMOTE_ADDR")
# define    VAR_HTTP_RMT_HOST   ERx("REMOTE_HOST")
# define    VAR_HTTPS           ERx("HTTPS")
# define    VAR_SECURE_PORT     ERx("SERVER_PORT_SECURE")

# ifdef UNIX
# ifdef HPUX
# define    API_TYPE            ERx("1.sl")
# else
# define    API_TYPE            ERx("1.so")
# endif /* HPUX */
# define    CGI_TYPE            ERx("cgi")
# else  /* UNIX */
# define    API_TYPE            ERx("dll")
# define    CGI_TYPE            ERx("exe")
# endif /* UNIX */

# define    BUFFER_SIZE         2048
# define    STR_SIZE            256
# define    NUMBER_SIZE         10
# define    STR_LENGTH_SIZE     3

# define    REMOTE_FILE         ERx("; filename=\"")
# define    CONTENT             ERx("Content-Type")
# define    DEPENDANCES         ERx("dependances")

typedef struct __ICEClient 
{
    PTR     user_info;
    char*   ContentType;
    char*   Data;
    i4      TotalBytes;
    char*   PathInfo;
    char*   Host;
    char*   Cookies;
    char*   QueryString;
    i4      cbAvailable;
    char*   gatewayType;
    char*   AuthType;
    char*   port;
    char*   agent;
    char*   rmt_addr;
    char*   rmt_host;
    char*   secure;
    char*   scheme;
    char*   node;
    u_i4   apistatus;

   GSTATUS (*initPage)  (char*,i4,struct __ICEClient*);
   GSTATUS (*sendURL)   (char*,i4,struct __ICEClient*);
   GSTATUS (*write)     (u_i4, char*,struct __ICEClient*);
   GSTATUS (*read)      (u_i4*, char*,struct __ICEClient*);
} ICEClient, *PICEClient;

# define ICE_ERROR_CONTENT      ERx("Content-Type: text/html\n\n")
# define ICE_ERROR_PAGE         ERx("<html><body>CLIENT ERROR:<br>%s</body></html><br>")

/* Common ADI and NSAPI server variable names */
# define APIVAR_PATH_INFO       ERx("path-info")
# define APIVAR_CONTENT_LENGTH  ERx("content-length")
# define APIVAR_CONTENT_TYPE    ERx("content-type")
# define APIVAR_QUERY           ERx("query")
# define APIVAR_AGENT           ERx("agent")
# define APIVAR_RMT_ADDR        ERx("REMOTE_ADDR")
# define APIVAR_RMT_HOST        ERx("REMOTE_HOST")
# define APIVAR_HOST            ERx("host")
# define APIVAR_COOKIE          ERx("cookie")
# define APIVAR_METHOD          ERx("method")
# define APIVAR_LOCATION        ERx("Location")

/* NSAPI server variable names */
# define NSAPIVAR_AUTHORIZATION     ERx("authorization")
# define NSAPIVAR_PATH_INFO         APIVAR_PATH_INFO
# define NSAPIVAR_CONTENT_LENGTH    APIVAR_CONTENT_LENGTH
# define NSAPIVAR_CONTENT_TYPE      APIVAR_CONTENT_TYPE
# define NSAPIVAR_QUERY             APIVAR_QUERY
# define NSAPIVAR_AGENT             ERx("user-agent")
# define NSAPIVAR_RMT_ADDR          ERx("ip")
# define NSAPIVAR_HOST              APIVAR_HOST
# define NSAPIVAR_COOKIE            APIVAR_COOKIE
# define NSAPIVAR_METHOD            APIVAR_METHOD
# define NSAPIVAR_LOCATION          APIVAR_LOCATION

/* ADI server variable names */
# define ADAPIVAR_AUTHORIZATION   ERx("Authorization")
# define ADAPIVAR_PATH_INFO       APIVAR_PATH_INFO
# define ADAPIVAR_CONTENT_LENGTH  ERx("Content-Length")
# define ADAPIVAR_CONTENT_TYPE    ERx("Content-Type")
# define ADAPIVAR_QUERY           APIVAR_QUERY
# define ADAPIVAR_AGENT           ERx("User-Agent")
# define ADAPIVAR_RMT_ADDR        APIVAR_RMT_ADDR
# define ADAPIVAR_RMT_HOST        APIVAR_RMT_HOST
# define ADAPIVAR_HOST            ERx("Host")
# define ADAPIVAR_COOKIE          APIVAR_COOKIE

/* Apache server variable names */
# define APAPIVAR_AUTHORIZATION   ERx("Authorization")
# define APAPIVAR_PATH_INFO       APIVAR_PATH_INFO
# define APAPIVAR_CONTENT_LENGTH  ERx("Content-Length")
# define APAPIVAR_CONTENT_TYPE    ERx("Content-Type")
# define APAPIVAR_QUERY           APIVAR_QUERY
# define APAPIVAR_AGENT           ERx("User-Agent")
# define APAPIVAR_RMT_ADDR        APIVAR_RMT_ADDR
# define APAPIVAR_RMT_HOST        APIVAR_RMT_HOST
# define APAPIVAR_HOST            ERx("Host")
# define APAPIVAR_COOKIE          ERx("Cookie")

# define STR_POST_METHOD        ERx("POST")
# define STR_GET_METHOD         ERx("GET")
# define STR_BASIC_AUTH         ERx("Basic ")
# define HEADER_END             ERx("\n\n");

bool
ICEClientInitialize();

bool
ICEProcess (PICEClient client);

void
ICEClientTerminate();

#endif
