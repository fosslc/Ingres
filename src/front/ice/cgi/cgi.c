/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: cgi.c
**
PROGRAM = oiice.cgi

DEST = icebin

NEEDLIBS = C_APILIB ICECLILIB DDFLIB LIBINGRES

** Description:
**      Entry functions for performing cgi.
**
** History:
**      08-Oct-98 (fanra01)
**          Add history.
**          Initialise client.cbAvailable to 0 if no client data.
**      02-Nov-98 (fanra01)
**          Add remote addr and remote host.
**      10-Dec-1998 (fanra01)
**          Initialise client structure before use and force flush of writes.
**          Scan host for port number and truncate as the port variable should
**          give port number.
**      18-Jan-1999 (fanra01)
**          Add ming hints.
**          Fix compiler warnings on unix.
**      09-Apr-1999 (peeje01)
**          Add DEST ming hint
**	08-aug-1999 (mcgem01)
**	    Change nat and longnat to i4.
**      05-May-2000 (fanra01)
**          Add comment history for
**          Bug 100312
**          Add passing of url scheme for secure sockets.
*/
#include <compat.h>
#include "httpext.h"
#include <iceclient.h>

# define HTTP_REDIRECT_HEADER   ERx("Location: ")

#define AVAILABLE_MAX	32768

#define CGI_INPUT   ((stdinput == NULL) ? stdin : stdinput)

FILE*   stdinput = NULL;

static char *schemes[] = { "http", "https", NULL };
# define NUM_SCHEMES    sizeof(schemes)/sizeof(char*)

GSTATUS
CGIInitPage (
	char*   pszCType,
	i4      dwWritten,
	PICEClient client)
{
	GSTATUS err = GSTAT_OK;
	STATUS	status = OK;
	if ((status = SIwrite (dwWritten, pszCType, &dwWritten, stdout)) != OK)
		err = DDFStatusAlloc(status);
return(err);
}

GSTATUS 
CGISendURL (
	char* url,
	i4      dwWritten,
	PICEClient client)
{
	GSTATUS err = GSTAT_OK;
	STATUS	status = OK;
	i4 		len;
	status = SIwrite (STlength(HTTP_REDIRECT_HEADER), HTTP_REDIRECT_HEADER, &len, stdout);
	if (status == OK)
		status = SIwrite (dwWritten, url, &dwWritten, stdout);
	if (status == OK)
		status = SIwrite (2, "\n\n", &len, stdout);

	if (status != OK)
		err = DDFStatusAlloc(status);
return(err);
}

void
CGIGetVariable(
	char*	name, 
	char*	*value) 
{
  char* v = NULL;
	u_i4  length = 0;

	*value = NULL;

  NMgtAt (name, &v);
	if (v && *v)
	{
		length = STlength(v);
		*value = (char*) MEreqmem(0, length + 1, TRUE, NULL);
		if (*value != NULL)
			MECOPY_CONST_MACRO (v, length, *value);
    	(*value)[length] = EOS;
	}
}

GSTATUS
CGIWriteClient(
	u_i4 	len, 
	char*	buffer,
	PICEClient client) 
{
	GSTATUS err = GSTAT_OK;
	STATUS	status = OK; 
	status = SIwrite (len, buffer, (i4 *)&len, stdout);
	if (status != OK)
		err = DDFStatusAlloc(status);
        else
            SIflush (stdout);
return(err);
}

GSTATUS
CGIReadClient(
	u_i4 *len, 
	char *buffer,
	PICEClient client) 
{
	GSTATUS err = GSTAT_OK;
	STATUS	status = OK;
	status = SIread (CGI_INPUT, *len, (i4*)len, buffer);
	if (status != OK)
		err = DDFStatusAlloc(status);
return(err);
}

/* CGI defined entry points */

int
main (i4 argc, char** argv)
{
    ICEClient   client;
    u_i4        length = 0;
    char*       content_length = NULL;
    u_i4        status;
    char*       mssecure = NULL;
    char*       nssecure = NULL;
    char        secureflg[2];

    {
        char * pSleep;

        NMgtAt("ICE_DEBUG_SLEEP", &pSleep);
        if (pSleep && *pSleep)
        {
            i4 n;
            CVan(pSleep, &n);
            PCsleep (n*1000);
        }
    }

    MEfill (sizeof (ICEClient), 0, &client);

    ICEClientInitialize();

    client.user_info = NULL;
    client.scheme = schemes[0]; /* set default scheme to http */
    secureflg[0] = '0';
    secureflg[1] = 0;
    /*
    ** Microsoft returns character '1' for secure '0' for normal
    */
    CGIGetVariable( VAR_SECURE_PORT, &mssecure );
    if (mssecure && *mssecure)
    {
        client.scheme = (mssecure && *mssecure) ?
            schemes[(mssecure[0] - '0')] : schemes[0];
        client.secure = mssecure;
    }
    else
    {
        /*
        ** NetScape sets HTTPS env variable to "ON" for https
        */
        CGIGetVariable( VAR_HTTPS, &nssecure );
        if (nssecure && *nssecure)
        {
            if (STbcompare( "ON", 0, nssecure, 0, TRUE ) == 0)
            {
                secureflg[0] = '1';
                client.scheme = schemes[1];
            }
        }
        client.secure = secureflg;
    }
    CGIGetVariable(VAR_CONTENT_TYPE, &client.ContentType);
    CGIGetVariable(VAR_PATH_INFO, &client.PathInfo);

    CGIGetVariable(VAR_CONTENT_LENGTH, &content_length);
    if (content_length != NULL &&
        CVan(content_length, &client.TotalBytes) != OK)
        client.TotalBytes = 0;

    if (client.TotalBytes != 0)
    {
        client.cbAvailable = (client.TotalBytes > 0 && client.TotalBytes < AVAILABLE_MAX) ?
                                    client.TotalBytes : AVAILABLE_MAX;
        client.Data = (char*) MEreqmem(0, client.cbAvailable + 1, TRUE, NULL);
        if (client.Data != NULL)
            CGIReadClient((u_i4 *)&client.cbAvailable, client.Data, NULL);
        client.Data[client.cbAvailable] = '\0';

        if (client.TotalBytes == -1)
            client.TotalBytes = client.cbAvailable;
    }
    else
    {
        client.cbAvailable = 0;
        client.Data = NULL;
    }

    CGIGetVariable(VAR_QUERY_STRING, &client.QueryString);
    CGIGetVariable(VAR_HTTP_HOST, &client.Host);
    /*
    ** Scan host for port and remove it
    */
    if (client.Host != NULL)
    {
        char* p = client.Host;
        while (*p != EOS)
        {
            if (*p == ':')
                *p = EOS;
            else
                CMnext(p);
        }
    }
    CGIGetVariable(VAR_HTTP_COOKIE, &client.Cookies);
    CGIGetVariable(VAR_HTTP_AUTH, &client.AuthType);
    CGIGetVariable(VAR_SERVER_PORT, &client.port);
    CGIGetVariable(VAR_HTTP_AGENT, &client.agent);
    CGIGetVariable(VAR_HTTP_RMT_ADDR, &client.rmt_addr);
    CGIGetVariable(VAR_HTTP_RMT_HOST, &client.rmt_host);

    client.gatewayType = CGI_TYPE;

    client.initPage = CGIInitPage;
    client.sendURL = CGISendURL;
    client.write = CGIWriteClient;
    client.read = CGIReadClient;

    status = (ICEProcess (&client) == TRUE) ? 0 : 1;

    ICEClientTerminate();

    MEfree((PTR)client.Data);
    return(status);
}
