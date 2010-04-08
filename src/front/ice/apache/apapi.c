/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: apapi.c
**
** Description:
**      Module contains the entry functions for Apache extensions.
**
** History:
**      19-Jan-2000 (fanra01)
**          Created.
**	08-Feb-2000 (hanch04)
**	    Include Ingres header files before Apache headers.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.
**          Take scheme information from HTTP request and pass it to ice
**          request.
**      05-Jul-2000 (fanra01)
**          Bug 102051
**          Add test for no scheme pointer in parsed uri.
**      21-Nov-2000 (fanra01)
**          Bug 103276
**          During URL redirection the contents of newpath is random and
**          a non-zero value causes the pointer to be tested in GAlloc causing
**          an exception.
**          Add return of Apache API status when set to a value other than
**          HTTP_OK e.g. REDIRECT.
**      20-Dec-2000 (mosjo01)
**          Had to include stdlib.h first to avoid syntax error with "abs".
**      31-Jan-2001 (bonro01)
**	    compat.h needed for dgi_us5 to properly expand posix functions
**	    in pthread.h that is indirectly included as part of iceclient.h
**	    bzarch.h contains a special define for posix threads that
**	    indicates the specific draft of posix support that is being
**	    compiled.
**	    Undefined _FILE_OFFSET_BITS to turn off Large file support
**	    for Apache.  If a customer turns on Large file support in the
**	    Apache build, then this will cause an abend in the Apache
**	    server.  In ICE_ProcessRequest the request_rec structure
**	    changes size depending on the size of the stat structure
**	    which is dependent on the _FILE_OFFSET_BITS variable.
**	18-jul-2001 (hayke02)
**	    Modify above change (part of wansh01's ingres!ingres25 change
**	    449972) so that the #include of compat.h is ifdef'ed dgi_us5.
**      28-Mar-2001 (fanra01)
**          Bug 104362
**          Modified the method of retrieving the content-type from the
**          request headers.
**      21-Mar-2002 (fanra01)
**          Bug 107379
**          Modified error reporting in ap_writeclient to not change
**          apistatus unless a write error occurs.  This enables the error
**          status to be returned to the web server.
**      01-may-2002 (wanfr01)
**          Modify hayke02's change to include bzarch.h.  Without bzarch.h
**          dgi_us5 isn't defined. (Bug 107793)
*/

# include <iceclient.h>
# include <ddfcom.h>

/*
** Include files from Apache 1.3.9 build
*/
#include <bzarch.h> 
#ifdef dgi_us5
# include <compat.h>
#undef _FILE_OFFSET_BITS
#endif

# include <stdlib.h>
# include "httpd.h"
# include "http_core.h"
# include "http_config.h"
# include "http_log.h"
# include "http_main.h"
# include "http_protocol.h"
# include "util_script.h"

# define MAX_NUM_LEN    30

static int ICE_Init( request_rec* r );

static int ICE_ProcessRequest ( request_rec* r );

static void ICE_Terminate( server_rec* s, pool* p );

static bool iceinitialized = FALSE;

/*
** supported schemes for normal and secure sockets.
*/
static char *schemes[] = { "http", "https", NULL };
# define NUM_SCHEMES    sizeof(schemes)/sizeof(char*)

/*
** Name: APAPIInitPage
**
** Description:
**      Takes the HTTP headers returned from the ice server and updates
**      the entries in the request output headers.
**      N.B.    The content-type must be set by assigning the value to
**              the request_rec content type field.
**
** Inputs:
**      type    pointer to buffer containing header types.
**      written number of bytes to be written in buffer type.
**      client  pointer to ice client structure for this request session.
**
** Outputs:
**      r->headers_out  updated.
**
** Returns:
**      GSTAT_OK    There is no failure returned.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
*/
GSTATUS
APAPIInitPage ( char* type, i4 written, PICEClient client )
{
    GSTATUS status = GSTAT_OK;
    request_rec* r = (request_rec*)client->user_info;
    char *name, *value;
     

    client->apistatus = HTTP_OK;

    /*
    ** scan through the buffer and set each header defined in the
    ** output headers.
    */
    while (written > 0)
    {
        name = type;
        while (written > 0 && type[0] != ':')
        {
            type++;
            written--;
        }
        if (type[0] == ':')
        {
            type[0] = EOS; 
            CMnext(type);
            while (CMwhite(type))
            {
                CMbytedec (written, type);
                CMnext (type);
            }
            value = type;
            written--;
            while (written > 0 && type[0] != '\n')
            {
                type++;
                written--;
            }
            if (type[0] == '\n')
            {
                type[0] = EOS;
                type++; written--;
                if (STbcompare( name, 0, APAPIVAR_CONTENT_TYPE, 0, TRUE) != 0)
                {
                    /*
                    ** replace or set the header
                    */
                    ap_table_setn(r->headers_out, name, value);
                }
                else
                {
                    r->content_type = value;
                }
            }
        }
    }
    ap_soft_timeout("send ice header", r);

    ap_send_http_header(r);

    if (r->header_only)
    {
        ap_kill_timeout(r);
    }
    return(status);
}

/*
** Name: APAPISendURL
**
** Description:
**      Return the redirected address to be displayed.
**
** Inputs:
**      url     address of the page.
**      written length of the url string.
**      client  pointer to ice client structure for this request session.
**
** Outputs:
**      r->method
**      r->method_number
**      r->headers_out
**
** Returns:
**      GSTAT_OK    there is no failure returned.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
**      21-Nov-2000 (fanra01)
**          Bug 103276
**          Initialise newpath as GAlloc will test a non-zero object to
**          determine size.
*/
GSTATUS
APAPISendURL ( char* url, i4 written, PICEClient client )
{
    GSTATUS status = GSTAT_OK;
    request_rec* r = (request_rec*)client->user_info;
    char*   newpath = NULL;

    if ((status = GAlloc( (PTR*)&newpath, written + 1, FALSE )) == GSTAT_OK)
    {
        MECOPY_VAR_MACRO( url, written, newpath );
        newpath[written] = '\0';
        r->method = "GET";
        r->method_number = M_GET;
        ap_table_setn( r->headers_out, ERx("Location"), newpath );
        MEfree ( newpath );
        /*
        ** Return no action to the server as correct response.
        */
        client->apistatus = REDIRECT;
    }
    else
    {
        client->apistatus = HTTP_INTERNAL_SERVER_ERROR;
    }

    return(status);
}

/*
** Name: APAPIWriteClient
**
** Description:
**      Return the generated page to the client.
**
** Inputs:
**      len     length of the page held in buffer.
**      buffer  pointer to the generated page.
**      client  pointer to ice client structure for this request session.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    there is no failure returned.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
**      21-Mar-2002 (fanra01)
**          Modified error check to not change the value of the apistatus
**          unless an error occurs on write.
*/
GSTATUS APAPIWriteClient( u_i4 len, char *buffer, PICEClient client )
{
    GSTATUS status = GSTAT_OK;
    request_rec* r = (request_rec*)client->user_info;
    int retval;

    retval = ap_rwrite(buffer, len, r);

    if (retval < 0)
    {
        client->apistatus =  retval;
    }
    return(status);
}

/*
** Name: APAPIReadClient
**
** Description:
**      Read input from the client  into the specified buffer.
**
** Inputs:
**      len     size of the target area.
**      buffer  pointer to the target area.
**      client  pointer to ice client structure for this request session.
**
** Outputs:
**      len     actual number of bytes copied to the target area.
**
** Returns:
**      GSTAT_OK    there is no failure returned.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
*/
GSTATUS
APAPIReadClient( u_i4 *len, char *buffer, PICEClient client )
{
    GSTATUS         status = GSTAT_OK;
    request_rec*    r = (request_rec*)client->user_info;
    char            argsbuf[HUGE_STRING_LEN];
    int             readlen = 0;

    if ((client->apistatus =
        ap_setup_client_block( r, REQUEST_CHUNKED_ERROR )) == OK )
    {
        readlen = ap_get_client_block( r, argsbuf, *len );
        if (readlen > 0)
        {
            MECOPY_VAR_MACRO( argsbuf, readlen, buffer );
            *len = readlen;
        }
    }
    return(status);
}

/*
** Name: ICE_Init
**
** Description:
**      Function invokes client initialization.
**
** Inputs:
**      r   pointer to request_rec structure ignored.
**
** Outputs:
**      None.
**
** Returns:
**      DECLINED    for the request to continue.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
*/
static
int ICE_Init( request_rec* r )
{
    if (iceinitialized == FALSE)
    {
        ICEClientInitialize();
        iceinitialized = TRUE;
    }
    return(DECLINED);
}

/*
** Name: ICE_ProcessRequest
**
** Description:
**      Function extracts the information required to form an ice request
**      from the request_rec structure.
**
** Inputs:
**      r   request_rec structure for the request.
**
** Outputs:
**      None.
**
** Returns:
**      DECLINED                    request does not contain an ice reference.
**      OK                          request has been handled.
**      HTTP_INTERNAL_SERVER_ERROR  an error has been returned from iceclient
**                                  library.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created
**      21-Nov-2000 (fanra01)
**          Add return of Apache API status when set to a value other than
**          HTTP_OK e.g. REDIRECT.
**      28-Mar-2001 (fanra01)
**          Change the method of retrieving content-type information from the
**          request header.
*/
static int
ICE_ProcessRequest( request_rec* r )
{
    int         retval = OK;
    int         readlen = 0;
    ICEClient   client;
    char argsbuf[HUGE_STRING_LEN];
    GSTATUS     status = GSTAT_OK;
    char        resbuf[MAX_NUM_LEN];
    char*       tmp;
    char*       host;
    char*       path;
    char        secure[2];

    MEfill (sizeof (ICEClient), 0, &client);

    if ((retval = ap_setup_client_block( r, REQUEST_CHUNKED_ERROR )))
        return retval;

    if (((path = STstrindex( r->uri, "oiiceapapi", 0, TRUE )) == NULL) &&
        ((path = STstrindex( r->uri, "oiice", 0, TRUE )) == NULL))
    {
        return(DECLINED);
    }

    client.user_info = (PTR) r;
    client.Data = NULL;

    /*
    ** Get the parameters from the request
    */
    if (ap_should_client_block( r ))
    {
        while((readlen =
            ap_get_client_block( r, argsbuf, HUGE_STRING_LEN )) > 0)
        {
            /*
            ** GAlloc will preserve the buffer when extending.
            */
            status=GAlloc( &client.Data, client.cbAvailable + readlen, TRUE );
            if (status == GSTAT_OK)
            {
                char* p = client.Data + client.cbAvailable;
                MECOPY_VAR_MACRO( argsbuf, readlen, p );
                client.cbAvailable += readlen;
            }
        }
    }

    tmp = (char *)ap_table_get( r->headers_in, APAPIVAR_CONTENT_LENGTH );
    if (tmp == NULL || CVan(tmp, &client.TotalBytes) != OK)
    {
        client.TotalBytes = 0;
    }
    if (client.TotalBytes < client.cbAvailable)
    {
        client.cbAvailable = client.TotalBytes;
    }

    /*
    ** Change the method of retrieving the content-type from the
    ** request header.
    */
    client.ContentType = (char *)ap_table_get( r->headers_in,
        APAPIVAR_CONTENT_TYPE );
    if ((tmp = STstrindex( (char *)r->path_info, API_TYPE, 0, TRUE )) != NULL)
    {
        /*
        ** From the api type scan forward to the first forward slash.
        ** This is the first character of path
        */
        if ((tmp = STindex( tmp, "/", 0 )) == NULL)
        {
            tmp = (char *)r->path_info;
        }
    }
    else
    {
        tmp = (char *)r->path_info;
    }
    client.PathInfo = tmp;
    client.QueryString = (char *)r->args;
    client.agent = (char *)ap_table_get( r->headers_in, APAPIVAR_AGENT );
    client.rmt_addr = (char *)r->connection->remote_ip;
    client.rmt_host = (char *)ap_get_remote_host(r->connection, r->per_dir_config,
        REMOTE_NAME); 
    client.Host = (char *)ap_table_get( r->headers_in, APAPIVAR_HOST );

    if ((client.Host) && ((host = STindex( client.Host, ":", 0 )) != NULL))
    {
        *host = EOS;
    }

    ap_snprintf( resbuf, sizeof(resbuf), "%u", ap_get_server_port( r ) );
    client.port = resbuf;

    client.AuthType = (char *)r->connection->ap_auth_type;

    client.Cookies = (char *)ap_table_get( r->headers_in, APAPIVAR_COOKIE );

    client.gatewayType = API_TYPE;

    /*
    ** set the requested scheme
    */
    client.scheme = r->parsed_uri.scheme;

    /*
    ** scan the schemes and the set the secure flag if secure sockets used
    */
    secure[0] = '0';
    if ((client.scheme != NULL) &&
        (STbcompare(schemes[1], 0, client.scheme, 0, TRUE) == 0))
    {
        secure[0] = '1';
    }
    secure[1] = 0;
    client.secure = secure;

    client.initPage = APAPIInitPage;
    client.sendURL = APAPISendURL;
    client.write = APAPIWriteClient;
    client.read = APAPIReadClient;

    /*
    ** if the apistatus is not 0 return the apistatus value.
    */
    if (ICEProcess (&client) != TRUE)
    {
        return ((client.apistatus != 0) ? client.apistatus :
            HTTP_INTERNAL_SERVER_ERROR);
    }
    /*
    ** If the status is anything other than HTTP_OK return it otherwise
    ** return OK
    */
    return( (client.apistatus != HTTP_OK) ? client.apistatus : OK);
}

/*
** Name: ICE_Terminate
**
** Description:
**      Terminates the extension and releases client resources.
**
** Inputs:
**      s   server_rec structure    ingnored.
**      p   pool memory             ignored.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      21-Jan-2000 (fanra01)
**          Created.
*/
static void
ICE_Terminate( server_rec* s, pool* p  )
{
    if (iceinitialized == TRUE)
    {
        ICEClientTerminate();
        iceinitialized = FALSE;
    }
}

/*
** content handlers available from this module
**
** in the httpd.conf file add a location
**
**  <Location /ice-bin>
**      SetHandler  ice-ext
**  </Location>
**
** when a location of /ice-bin is found in a uri the handler described by
** ice-ext is invoked.
*/
static const handler_rec ice_handlers[] =
{
    { "ice-ext", ICE_ProcessRequest },
    { NULL }
};

/*
** module definition
**
** in the httpd.conf file
**
** for NT append to the LoadModule section
**
**      LoadModule ice_module modules/oiiceapapi.dll
**
** for unix append to the LoadModule section
**
**      LoadModule ice_module libexec/oiiceapapi.1.so
**
** and append to the AddModule section
**
**      AddModule   apapi.c
*/
module MODULE_VAR_EXPORT ice_module =
{
    STANDARD_MODULE_STUFF,
    NULL,                       /* initializer */
    NULL,                       /* dir config creater */
    NULL,                       /* dir merger --- default is to override */
    NULL,                       /* server config */
    NULL,                       /* merge server config */
    NULL,                       /* command table */
    ice_handlers,               /* handlers */
    NULL,                       /* filename translation */
    NULL,                       /* check_user_id */
    NULL,                       /* check auth */
    NULL,                       /* check access */
    NULL,                       /* type_checker */
    NULL,                       /* fixups */
    NULL,                       /* logger */
    NULL,                       /* header parser */
    NULL,                       /* child_init */
    ICE_Terminate,              /* child_exit */
    ICE_Init                    /* [1] post read_request handling */
};
