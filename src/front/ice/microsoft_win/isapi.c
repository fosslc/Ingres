/*
** Copyright (C) 2004 Ingres Corporation All Rights Reserved.
*/
/*
** Name: isapi.c
**
** Description:
**      Module contains the entry functions for both microsoft and netscape
**      extensions.
**
**      ISAPIInitPage
**      ISAPISendURL
**      ISAPIWriteClient
**      ISAPIReadClient
**      GetExtensionVersion
**      ISAPIGetVariable
**      HttpExtensionProc
**      TerminateExtension
**      ICE_Init
**      ICE_ProcessRequest
**      ICE_Terminate
**
** History:
**      02-Nov-98 (fanra01)
**          Add remote address and remote host.
**      16-Nov-98 (fanra01)
**          Moved the registration with the netscape termination into
**          netscape initialisation function.
**      10-Dec-1998 (fanra01)
**          Add initialisation of ICEClient structure.
**          Remove port from hostname if it exists.
**          Correct error return from extension procedure. Apache and Spyglass
**          exception on error.
**      05-Jan-1999 (fanra01)
**          Add a null terminator to the redirected url.
**	09-aug-1999 (mcgem01)
**	    Change nat and longnat to i4.
**      26-Mar-1999 (fanra01)
**          Make a copy of the target url and ensure that the memory has space
**          for a null terminator.
**      25-Jan-2000 (fanra01)
**          Bug 100145
**          Update name and location of the netscape extension.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.
**          Take scheme information from HTTP request and pass it to ice
**          request.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#include "httpext.h"
#include <iceclient.h>


#define ICE_ISAPI_DESCRIPTION ERx("ICE Server 2.5 ISAPI Web Server Extension")

static char *schemes[] = { "http", "https", NULL };
# define NUM_SCHEMES    sizeof(schemes)/sizeof(char*)

u_i4 ICE_Terminate (PTR data);

GSTATUS 
ISAPIInitPage (
	char*   pszCType,
	i4      dwWritten,
	PICEClient client)
{
	GSTATUS err = GSTAT_OK;
	EXTENSION_CONTROL_BLOCK*    pecb = (EXTENSION_CONTROL_BLOCK*) client->user_info;

	if (pecb->ServerSupportFunction != NULL)
		if (pecb->ServerSupportFunction (pecb->ConnID,
                                        HSE_REQ_SEND_RESPONSE_HEADER,
			                                  ERx ("200 OK"), 
																				&dwWritten,
                                        (LPDWORD)pszCType) == FALSE)
			err = DDFStatusAlloc(GetLastError());
return(err);
}

/*
** Name: ISAPISendURL
**
** Description:
**      Function changes the page addressed by the browser by redirecting to
**      the new path.
**
** Input:
**      url         new path to redirect to.
**      dwWritten   length of new path.
**      client      ICE client structure.
**
** Output:
**      None.
**
** History:
**      05-Jan-1999 (fanra01)
**          Ensure that the new path is null terminated.
**      26-Mar-1999 (fanra01)
**          Make a copy of the target url and ensure that the memory has space
**          for a null terminator.
*/
GSTATUS
ISAPISendURL (
    char*       url,
    i4          dwWritten,
    PICEClient  client)
{
    GSTATUS err = GSTAT_OK;
    EXTENSION_CONTROL_BLOCK*    pecb = (EXTENSION_CONTROL_BLOCK*) client->user_info;
    char*   newpath = NULL;

    if ((err = GAlloc ((PTR*)&newpath, dwWritten+1, FALSE)) == GSTAT_OK)
    {
        MECOPY_VAR_MACRO(url, dwWritten, newpath);
        newpath[dwWritten] = '\0';

        if (pecb->ServerSupportFunction != NULL)
            if (pecb->ServerSupportFunction (pecb->ConnID,
                                            HSE_REQ_SEND_URL_REDIRECT_RESP,
                                            newpath,
                                            &dwWritten,
                                            NULL) == FALSE)
                err = DDFStatusAlloc(GetLastError());

        MEfree(newpath);
    }
    return(err);
}

GSTATUS
ISAPIWriteClient(
	u_i4  len, 
	char *buffer,
	PICEClient client)
{
	GSTATUS err = GSTAT_OK;
	EXTENSION_CONTROL_BLOCK*    pecb = (EXTENSION_CONTROL_BLOCK*) client->user_info;
	if (pecb->WriteClient != NULL)
		if (pecb->WriteClient (pecb->ConnID, (LPVOID)buffer, (LPDWORD)&len, 0L) == FALSE)
			err = DDFStatusAlloc(GetLastError());
return(err);
}

GSTATUS
ISAPIReadClient(
	u_i4  *len, 
	char *buffer,
	PICEClient client)
{
	GSTATUS err = GSTAT_OK;
	EXTENSION_CONTROL_BLOCK*    pecb = (EXTENSION_CONTROL_BLOCK*) client->user_info;

	if (pecb->ReadClient == NULL)
		*len = 0;
	else
		if (pecb->ReadClient (pecb->ConnID, (LPVOID)buffer, (LPDWORD)len) == FALSE)
		{
			err = DDFStatusAlloc(GetLastError());
			*len = 0;
		}
return(err);
}

/* ISAPI defined entry points */

/*
** Name: GetExtensionVersion - required ISAPI entry point
**
** Description:
**      Called by ISAPI when the extension DLL is loaded. 
**
** Inputs:
**
** Outputs:
**      pver - struct containing version number and version string
** 
** Returns:
**      TRUE
**
** History:
*/
BOOL WINAPI
GetExtensionVersion (HSE_VERSION_INFO *pVer) 
{
	u_i4  length = 0;
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                         HSE_VERSION_MAJOR );

    STlcopy (ICE_ISAPI_DESCRIPTION,
             pVer->lpszExtensionDesc,
             HSE_MAX_EXT_DLL_NAME_LEN);

	ICEClientInitialize();
  return TRUE;
}

/*
** Name: HTTPExtensionProc - ISAPI main entry point
**
** Description:
**      This function is called by the Web Server when it receives a request
**      aimed at this ISAPI server extension.
**
** Inputs:
**      pecb - ptr to ISAPI extension control block.
**
** Outputs:
**      None
**
** Returns:
**      HSE_STATUS_SUCCESS.
**
** History:
*/
STATUS
ISAPIGetVariable(
	EXTENSION_CONTROL_BLOCK*    pecb, 
	char *name, 
	char *value, 
	u_i4  *length) 
{
	STATUS status = 0;
	value[0] = EOS;
	if (pecb->GetServerVariable != NULL)
	{
		if (pecb->GetServerVariable (pecb->ConnID, name, value, (LPDWORD) length) == FALSE)
			status = GetLastError();
	}
	else
		value[0] = EOS;
return (status);
}

DWORD WINAPI
HttpExtensionProc (EXTENSION_CONTROL_BLOCK* pecb)
{
    STATUS      status = 0;
    ICEClient   client;
    char        host[STR_SIZE];
    char        port[NUMBER_SIZE];
    char        cookies[STR_SIZE];
    char        auth[STR_SIZE];
    char        agent[STR_SIZE];
    char        rmt_addr[STR_SIZE];
    char        rmt_host[STR_SIZE];
    char        secure[NUMBER_SIZE];
    u_i4        length = 0;

    MEfill (sizeof (ICEClient), 0, &client);

    client.user_info = (PTR) pecb;
    client.ContentType = pecb->lpszContentType;
    client.Data = pecb->lpbData;
    client.TotalBytes = pecb->cbTotalBytes;
    client.PathInfo = pecb->lpszPathInfo;
    client.cbAvailable = pecb->cbAvailable;
    client.QueryString = pecb->lpszQueryString;
    client.gatewayType = API_TYPE;

    length = STR_SIZE;
    ISAPIGetVariable (pecb, VAR_HTTP_HOST, host, &length);
    client.Host = host;
    /*
    ** Scan host for port and remove it
    */
    if (host[0] != EOS)
    {
        char* p = host;
        while (*p != EOS)
        {
            if (*p == ':')
                *p = EOS;
            else
                CMnext(p);
        }
    }

    length = NUMBER_SIZE;
    status = ISAPIGetVariable (pecb, VAR_SERVER_PORT, port, &length);
    client.port = port;

    /*
    ** get the secure flag and set the scheme accordingly
    */
    length = NUMBER_SIZE;
    status = ISAPIGetVariable (pecb, VAR_SECURE_PORT, secure, &length);
    client.secure = secure;
    client.scheme = (secure[0] != 0) ? schemes[(secure[0] - '0')] : schemes[0];

    length = STR_SIZE;
    status = ISAPIGetVariable (pecb, VAR_HTTP_COOKIE, cookies, &length);
    client.Cookies = cookies;

    length = STR_SIZE;
    status = ISAPIGetVariable (pecb, VAR_HTTP_AUTH, auth, &length);
    client.AuthType = auth;

    length = STR_SIZE;
    status = ISAPIGetVariable (pecb, VAR_HTTP_AGENT, agent, &length);
    client.agent = agent;

    length = STR_SIZE;
    status = ISAPIGetVariable (pecb, VAR_HTTP_RMT_ADDR, rmt_addr, &length);
    client.rmt_addr = rmt_addr;

    length = STR_SIZE;
    status = ISAPIGetVariable (pecb, VAR_HTTP_RMT_HOST, rmt_host, &length);
    client.rmt_host = rmt_host;

    client.initPage = ISAPIInitPage;
    client.sendURL = ISAPISendURL;
    client.write = ISAPIWriteClient;
    client.read = ISAPIReadClient;

    if (ICEProcess (&client) != TRUE)
        return HSE_STATUS_ERROR;
    else
        return HSE_STATUS_SUCCESS;
}

/*
** Name: TerminateExtension - called to notify ICE that it is about to be
**                            unloaded
**
** Description:
**      This function is called by the Web Server before it unloads the ICE
**      DLL. 
**
** Inputs:
**      dwFlags - Specifies whether the termination is advisory, and can be
**                refused, or mandatory. We always unload, so ignored
**
** Outputs:
**      None.
**
** Returns:
**      TRUE, indicating that the unload may proceed.
**
** History:
**      28-nov-1996 (wilan06)
**          Created.
*/
BOOL WINAPI
TerminateExtension (DWORD dwFlags)
{
	ICEClientTerminate();
    return TRUE;
}

/*
** Name: ICE_Init - Switch to the Netscape API
**
** Description:
**
** Inputs:
**
** Outputs:
**      None.
**
** Returns:
**
** History:
**      28-nov-1996 (wilan06)
**          Created.
**      02-Oct-98 (fanra01)
**          Updated to reflect library name change.
**      12-Nov-98 (fanra01)
**          Updated to invoke client initialisation in the context of the
**          NSAPI dll.  Add exit for completeness.
**      25-Jan-2000 (fanra01)
**          Changed to reflect the new path of the netscape extension.
*/
static u_i4  ( *NSInitFct )  (PTR, PTR, PTR);
static u_i4  ( *NSEntryFct ) (PTR, PTR, PTR);
static u_i4  ( *NSExitFct ) (PTR);

u_i4 
ICE_Init (
	PTR pb,
	PTR sn,
	PTR rq)
{
    CL_ERR_DESC err_desc;
    PTR handle = NULL;
    char tmp[MAX_LOC+1];
    char tmploc[MAX_LOC+1];
    LOCATION loc;
    char *system = NULL;
    char *httpext = NULL;

    STprintf( tmp, "%s_SYSTEM",  SYSTEM_VAR_PREFIX );

    NMgtAt ( tmp, &system );
    if (system && *system)
    {
        STcopy( system, tmploc );
        LOfroms( PATH, tmploc, &loc );
        LOfaddpath( &loc, SYSTEM_LOCATION_SUBDIRECTORY, &loc );
        LOfaddpath( &loc, ERx("ice"), &loc );
        LOfaddpath( &loc, ERx("bin"), &loc );
        LOfaddpath( &loc, ERx("netscape"), &loc );

        if (DLprepare_loc ( NULL, "oiicensapi", NULL, &loc, 0, &handle,
            &err_desc) == OK)
        {
            if (DLbind ( handle, "ICE_Init", (PTR *) &NSInitFct, &err_desc) == OK)
            {
                if (DLbind ( handle, "ICE_ProcessRequest", (PTR *) &NSEntryFct,
                    &err_desc) == OK)
                {
                    if (DLbind ( handle, "ICE_Terminate", (PTR *) &NSExitFct,
                        &err_desc) == OK)
                    {
                        NSInitFct(pb, sn, rq);
                    }
                }
            }
        }
    }
    if ((NSInitFct != NULL) && (NSEntryFct != NULL) && (NSExitFct != NULL))
	return(0);
    else
	return(-1);
}

u_i4 
ICE_ProcessRequest (
	PTR	pb, 
	PTR sn, 
	PTR rq)
{
return(NSEntryFct(pb, sn, rq));
}

u_i4 
ICE_Terminate (PTR data)
{
return(NSExitFct(data));
}
