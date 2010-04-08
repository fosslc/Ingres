/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceclient.c
**
** Description:
**      Functions to provide connection from either an HTTP extension or from
**      a cgi.
**
** History:
**      15-Oct-98 (fanra01)
**          Changed name of trace variable.
**      02-Nov-1998 (fanra01)
**          Add remote addr and remote host.
**      10-Dec-1998 (fanra01)
**          Add trace messages for writing pages back to http server.
**      18-Jan-1999 (fanra01)
**          Fix compiler warnings on unix.
**      15-Feb-1999 (fanra01)
**          Add removal of saved session on failure.  This will allow a
**          reconnect without having to restart the http server.
**      26-Mar-1999 (fanra01)
**          Modified calculation of paramsSize in ICEQuery() as previous
**          result was always 1 causing memory corruption.
**      24-Jan-2000 (fanra01)
**          Bug 100121
**          Correct memory leak in client when generating returned page.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.
**      06-Jun-2000 (fanra01)
**          Bug 101345
**          Set the remote node name to connect to. Taken either from the
**          environment variable or from the connect parameter.
**          Allocate the receive buffer for blob reconstruction.
**          Rename global remote_note to rmt_svr_node.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-apr-2001 (mcgem01)
**	    Clean up wansh01's change which is illegal on NT.
**      21-Mar-2002 (fanra01)
**          Bug 107379
**          Ensure that a server error is returned when an error is reported
**          to the web server.
*/
#include <iceclient.h>
#include <icegca.h>
#include <mu.h>
#include <erddf.h>

#include <adf.h>
#include <adp.h>

#define SEMAPHORE		MU_SEMAPHORE
#define PSEMAPHORE	*MU_SEMAPHORE

#define DDFSemCreate(X, Y) (MUw_semaphore(X, Y) == OK)	?\
								 GSTAT_OK : \
								 DDFStatusAlloc ( E_DF0007_SEM_CANNOT_CREATE) 

#define DDFSemOpen(X, Y)	(MUp_semaphore(X) == OK) ? \
								 GSTAT_OK : \
								 DDFStatusAlloc( E_DF0008_SEM_CANNOT_OPEN)

#define DDFSemClose(X)		(MUv_semaphore(X) == OK) ? \
								 GSTAT_OK : \
								 DDFStatusAlloc( E_DF0009_SEM_CANNOT_CLOSE)

#define DDFSemDestroy(X)	(MUr_semaphore(X) == OK) ? \
								 GSTAT_OK : GSTAT_OK

static		PICEGCASession		sessionList = NULL;
static		SEMAPHORE					SessionIntegrity;
static          char* rmt_svr_node = NULL;

#define		SET_STR_PARAMS(X, Y)	\
		if (Y != NULL) STprintf(X, "%s%*d%s", X, STR_LENGTH_SIZE, STlength(Y), Y);\
		else STprintf(X, "%s%*d", X, STR_LENGTH_SIZE, 0)

#define		SET_NUM_PARAMS(X, Y) STprintf(X, "%s%*d", X, NUM_MAX_INT_STR, Y)

/*
** Name: ICEClientInitialize
**
** Description:
**      Read initialization variables and initiate the communications
**      interface.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      TRUE    Successfully completed.
**      FALSE   Failed.
**
** History:
**      07-Jun-2000 (fanra01)
**          Add history.
**          Initialize tran_place before a call DDFTraceInit.
*/
bool
ICEClientInitialize() 
{
    GSTATUS err = GSTAT_OK;
    char    *tran_place = NULL;
    char    *tran_level = NULL;
    i4     timeout = -1;
    char*   tmp = NULL;

    NMgtAt("II_ICE_CLIENT_TIMEOUT", &tmp);
    if (tmp != NULL && tmp[0] != EOS)
        CVan(tmp, &timeout);

    G_TRACE(1)("CLIENT -- Session timeout: %d", timeout);
    err = ICEGCAInitiate(timeout, NULL);

    if (err == GSTAT_OK)
    {
        u_i4    level = 0;

        /*
        ** Initialize trace with no level and no file location to
        ** setup some variables.
        */
        DDFTraceInit(level, tran_place);

        NMgtAt("II_ICE_CLIENT_TRACE", &tran_level);
        if (tran_level != NULL && tran_level[0] != EOS)
            CVan(tran_level, (i4 *)&level);

        NMgtAt("II_ICE_SERVER_NODE", &tmp);
        if (tmp != NULL && tmp[0] != EOS)
            rmt_svr_node = STalloc(tmp);

        NMgtAt("II_ICE_CLIENT_LOG", &tran_place);
        if (tran_place != NULL && tran_place[0] != EOS)
            DDFTraceInit(level, tran_place);

        err = DDFSemCreate(&SessionIntegrity, "ICEClient");
    }
    if (err == GSTAT_OK)
        return(TRUE);
    else
    {
        DDFStatusFree(TRC_EVERYTIME, &err);
        return FALSE;
    }
}

void 
ICEClientTerminate() 
{
	GSTATUS err = GSTAT_OK;
	PICEGCASession session;

	err = DDFSemOpen (&SessionIntegrity, TRUE);
	if (err == GSTAT_OK)
	{
		while (err == GSTAT_OK && sessionList != NULL) 
		{
			session = sessionList;
			sessionList = sessionList->next;
			CLEAR_STATUS(ICEGCADisconnect(session));
                        if (session->rcv != NULL)
                        {
                            MEfree( session->rcv );
                            session->rcv = NULL;
                        }
			MEfree((PTR) session);
		}
		CLEAR_STATUS(DDFSemClose(&SessionIntegrity));
	} 
	DDFSemDestroy(&SessionIntegrity);
	if (err != GSTAT_OK)
		DDFStatusFree(TRC_EVERYTIME, &err);
	MEfree((PTR)rmt_svr_node);
}

/*
** Name: ICEClientOpenConnection
**
** Description:
**      Establish a connection with a server.  If there is a connection
**      already available it is used.  Otherwise a new connection is
**      made.
**
** Inputs:
**      remote_node Node to connect to.  If NULL the rmt_svr_node is used.
**
** Outputs:
**      session     Session context structure.
**
** Returns:
**      GSTAT_OK    Successfully completed.
**      !0          Failed.
**
** History:
**      07-Jun-2000 (fanra01)
**          Add history.
**          Setup node for connection.  Add allocation of receive
**          reconstruction buffer for the session.
*/
GSTATUS
ICEClientOpenConnection(
    PICEGCASession  *session,
    char            *remote_node)
{
    GSTATUS err = GSTAT_OK;
    PICEGCASession tmp = NULL;
    char* node = NULL;

    err = DDFSemOpen (&SessionIntegrity, TRUE);
    if (err == GSTAT_OK)
    {
        tmp = sessionList;
        if (tmp != NULL)
        {
            sessionList = sessionList->next;
            tmp->next = NULL;
        }
        CLEAR_STATUS(DDFSemClose(&SessionIntegrity));
    }

    if (err == GSTAT_OK && tmp == NULL)
    {
        G_TRACE(6)("CLIENT -- Creating a new ICE Server connection");
        err = G_ME_REQ_MEM(0, tmp, ICEGCASession, 1);
        if (err == GSTAT_OK)
        {
            err = G_ME_REQ_MEM(0, tmp->rcv, char, 4096);
        }
        /*
        ** Call initiate with the address to store header length
        */
        if ((err == GSTAT_OK) && (tmp->gca_hdr_len == 0))
        {
            err = ICEGCAInitiate( -1, &tmp->gca_hdr_len );
            if (err != GSTAT_OK)
            {
		DDFStatusFree(TRC_EVERYTIME, &err);
            }
        }
        if (err == GSTAT_OK)
        {
            /*
            ** NULL node will assume a local node to connect to.
            ** Use remote node if set or use rmt_svr_node which
            ** will be local if not set in the environment.
            */
            node = (remote_node != NULL) ? remote_node :
                ((rmt_svr_node != NULL) ? rmt_svr_node : NULL);
            err = ICEGCAConnect(tmp, node);
        }
    }
    if (err == GSTAT_OK)
    {
        G_TRACE(6)("CLIENT -- Connection opened (node=%s)",
            (node != NULL) ? node : "local");
        *session = tmp;
    }
    else
    {
        G_TRACE(6)("CLIENT -- Unsuccessful connection (node=%s)",
            (node != NULL) ? node : "local");
        MEfree((PTR)tmp);
        *session = NULL;
    }
    return(err);
}

/*
** Name: ICEClientCloseConnection
**
** Description:
**      Return a connection to the connection pool.
**
** Inputs:
**      session     Session context structure.
**      usable      TRUE    connection is reusable.
**                  FALSE   disconnect session.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    Successfully completed.
**      !0          Failed.
**
** History:
**      07-Jun-2000 (fanra01)
**          Add history.
**          Add free of receive reconstruction area.
*/
GSTATUS
ICEClientCloseConnection(
    PICEGCASession session,
    bool                     usable)
{
    GSTATUS err = GSTAT_OK;

    err = DDFSemOpen (&SessionIntegrity, TRUE);
    if (err == GSTAT_OK)
    {
        G_TRACE(6)("CLIENT -- Disconnecting");
        if (usable == TRUE)
        {
            session->next = sessionList;
            sessionList = session;
        }
        else
        {
            err = ICEGCADisconnect(session);
            if (session->rcv)
            {
                MEfree( session->rcv );
            }
            MEfree( (PTR)session );
        }
        CLEAR_STATUS(DDFSemClose(&SessionIntegrity));
    }
return(err);
}

/*
** Name: ICEQuery
**
** Description:
**      Function takes input from the HTTP server and forms a message
**      containing query and query parmeters to send to the ice server.
**
** Inputs:
**      clientType  Type of client invoking request. HTML_API or C_API.
**      node        Node address of the invoking client.
**      client      Client context information.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    Successfully completed.
**      !0          Failed.
**
** History:
**      26-Mar-1999 (fanra01)
**          Add history.
**          Modified calculation of paramsSize as previous result was always 1.
**      24-Jan-2000 (fanra01)
**          Free memory allocated for storing the page.
**      28-Apr-2000 (fanra01)
**          Add tracing and handling for secure flag and scheme string.
**      12-Jun-2000 (fanra01)
**          Add sending of query data longer than a single gca message buffer
**          as continuation blob segments.
**
*/
GSTATUS
ICEQuery(
    u_i4        clientType,
    char*       node,
    PICEClient  client)
{
    GSTATUS         err = GSTAT_OK;
    PICEGCASession  session = NULL;
    SYSTIME         stime1, stime2;
    u_i4            stmtLength = 2*NUM_MAX_INT_STR + 10;
    u_i4            length = 0;
    TMnow(&stime1);

    G_TRACE(1)("CLIENT %x -- start request", client);
    if (client->ContentType != NULL)
    {
        G_TRACE(3)("CLIENT %x -- ContentType : %s", client, client->ContentType);
        stmtLength += STlength(client->ContentType) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->PathInfo != NULL)
    {
        G_TRACE(2)("CLIENT %x -- PathInfo : %s", client, client->PathInfo);
        stmtLength += STlength(client->PathInfo) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->Host != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Host : %s", client, client->Host);
        stmtLength += STlength(client->Host) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->port != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Port : %s", client, client->port);
        stmtLength += STlength(client->port) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->Cookies != NULL)
    {
        G_TRACE(2)("CLIENT %x -- Cookies : %s", client, client->Cookies);
        stmtLength += STlength(client->Cookies) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->AuthType != NULL)
    {
        G_TRACE(3)("CLIENT %x -- AuthType : %s", client, client->AuthType);
        stmtLength += STlength(client->AuthType) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->gatewayType != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Gateway : %s", client, client->gatewayType);
        stmtLength += STlength(client->gatewayType) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->agent != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Agent : %s", client, client->agent);
        stmtLength += STlength(client->agent) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->rmt_addr != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Remote Addr : %s", client,
                    client->rmt_addr);
        stmtLength += STlength(client->rmt_addr) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->rmt_host != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Remote Host : %s", client,
                    client->rmt_host);
        stmtLength += STlength(client->rmt_host) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->secure != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Secure : %s", client,
                    client->secure);
        stmtLength += STlength(client->secure) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    if (client->scheme != NULL)
    {
        G_TRACE(3)("CLIENT %x -- Scheme : %s", client,
                    client->scheme);
        stmtLength += STlength(client->scheme) + STR_LENGTH_SIZE;
    }
    else
        stmtLength += STR_LENGTH_SIZE;

    G_TRACE(3)("CLIENT %x -- cbAvailable : %d", client, client->cbAvailable);
    G_TRACE(3)("CLIENT %x -- TotalBytes : %d", client, client->TotalBytes);
    G_TRACE(3)("CLIENT %x -- Opening connection onto the ICE server (%s)",
        client, (node != NULL) ? node : "local");
    err = ICEClientOpenConnection(&session, node);
    if (err == GSTAT_OK)
    {
        char    *params = NULL;
        u_i4    paramsSize = client->cbAvailable;

        if (client->QueryString != NULL && client->QueryString[0] != EOS)
        {
            paramsSize += STlength( client->QueryString ) +
                ((client->TotalBytes > 0) ? 1 : 0);
        }

        length = (paramsSize > stmtLength) ? paramsSize: stmtLength;

        err = GAlloc(&params, length + 10, FALSE);
        if (err == GSTAT_OK)
        {
            params[0] = EOS;
            SET_NUM_PARAMS(params, clientType);
            SET_STR_PARAMS(params, client->ContentType);
            SET_STR_PARAMS(params, client->PathInfo);
            SET_STR_PARAMS(params, client->Host);
            SET_STR_PARAMS(params, client->port);
            SET_STR_PARAMS(params, client->Cookies);
            SET_STR_PARAMS(params, client->AuthType);
            SET_STR_PARAMS(params, client->gatewayType);
            SET_STR_PARAMS(params, client->agent);
            SET_STR_PARAMS(params, client->rmt_addr);
            SET_STR_PARAMS(params, client->rmt_host);
            SET_STR_PARAMS(params, client->secure);
            SET_STR_PARAMS(params, client->scheme);
            SET_NUM_PARAMS(params, (client->TotalBytes - client->cbAvailable) + paramsSize);
            if (paramsSize > 0)
            {
                SET_STR_PARAMS(params, "( ~V )");
            }
            G_TRACE(4)("CLIENT %x -- Sending request", client);
            err = ICEGCASendQuery (session, params, stmtLength, (paramsSize == 0));

            if (err == GSTAT_OK && paramsSize >0)
            {
                i4  position = 0;
                i4  read = 0;
                i4  totalsent = 0;
                i4  flags = 0;
                i4  len = 0;
                i4  remain = 0;
                i4  sent = 0;

                params[0] = EOS;
                G_TRACE(4)("CLIENT %x -- Format data", client);
                if (client->QueryString != NULL && client->QueryString[0] != EOS)
                {
                    position = STlength(client->QueryString);
                    if (position > 0)
                    {
                        MECOPY_VAR_MACRO(
                            client->QueryString,
                            position,
                            params);
                        if (client->cbAvailable > 0)
                            params[position++] = '&';
                    }
                    params[position] = EOS;
                    read = position;
                }

                /*
                ** if there is post/get data to send with the query
                */
                if (client->cbAvailable > 0)
                {
                    MECOPY_VAR_MACRO(
                        client->Data,
                        client->cbAvailable,
                        params + position);
                    position = client->cbAvailable;
                    read += client->cbAvailable;
                }

                /*
                ** While all the query data has not been sent
                ** send a segment at a time
                */
                do
                {
                    /*
                    ** Calculate how much room there is for data in the
                    ** send buffer, excluding segment indicator and
                    ** segment length.
                    */
                    len = GCA_BUFFER_SIZE - (0x18 + sizeof(i4) + sizeof(i2));

                    /*
                    ** If this is the first time set header flag
                    */
                    if (totalsent == 0)
                    {
                        flags |= ICE_QUERY_HEADER;
                        /*
                        ** Adjust for blob header
                        */
                        len += ADP_HDR_SIZE;

                        /*
                        ** Adjust for data type header: type, precision, length
                        */
                        len += sizeof(i2) + sizeof(i2) + sizeof(i4);
                    }
                    /*
                    ** If the remaining data fits into the send buffer
                    ** set the last message flag
                    */
                    if (read < len)
                    {
                        flags |= ICE_QUERY_FINAL;
                    }
                    /*
                    ** Do a send
                    */
                    G_TRACE(5)("CLIENT %x -- Sending data (%d/%d)", client, read, client->TotalBytes);
                    if ((err = ICEGCASendData( session, params, read, &flags,
                        &sent )) == GSTAT_OK)
                    {
                        /*
                        ** Turn off the header flag
                        */
                        flags &= ~ICE_QUERY_HEADER;

                        /*
                        ** Update total bytes sent
                        */
                        totalsent += sent;

                        /*
                        ** remain = read - sent;
                        ** keep remaining buffer
                        */
                        if ((remain = (read - sent)) > 0)
                        {
                            MECOPY_VAR_MACRO( params + sent, remain, params );
                        }
                        read -= sent;
                        if ((client->TotalBytes - totalsent) > remain)
                        {
                            /*
                            ** Set available read buffer size
                            */
                            u_i4 buflen = paramsSize - remain;

                            /*
                            ** Do a read
                            */
                            err = client->read(&buflen, params+remain, client);
                            read += buflen;
                        }
                    }
                }
                while (err == GSTAT_OK &&
                    ((client->TotalBytes > totalsent) ||
                    (flags & ICE_QUERY_FINAL)));
            }

            if (err == GSTAT_OK)
            {
                char *result = NULL;
                u_i4 returned = 0;
                u_i4 type_of_message;
                i4 moredata;
                G_TRACE(4)("CLIENT %x -- Waiting....", client);

                err = ICEGCAWait(session, &type_of_message);
                if (err == GSTAT_OK)
                {
                    G_TRACE(5)("CLIENT %x -- Reading header", client);
                    err = ICEGCAHeader(session, &result, &returned);
                    if (err == GSTAT_OK)
                    {
                        if (returned > 0)
                            G_TRACE(5)("CLIENT %x -- header [%s]", client, result);
                        switch (type_of_message)
                        {
                            case WPS_URL_BLOCK:
                                if (result != NULL)
                                {
                                    MEfree( result );
                                    result = NULL;
                                }
                                err = ICEGCAPage(session, &result, &returned, &moredata);
                                if (err == GSTAT_OK && (returned > 0 || moredata))
                                    err = client->sendURL(result, returned, client);
                                break;
                            default:
                                err = client->initPage (result, returned, client);
                                if (err == GSTAT_OK)
                                {
                                    if (result != NULL)
                                    {
                                        MEfree( result );
                                        result = NULL;
                                    }
                                    G_TRACE(5)("CLIENT %x -- Reading page", client);
                                    do
                                    {
                                        err = ICEGCAPage(session, &result, &returned, &moredata);
                                        if (err == GSTAT_OK && returned > 0)
                                        {
                                            G_TRACE(5)("CLIENT %x -- Read (%d)", client, returned);
                                            err = client->write(returned, result, client);
                                            if (err == GSTAT_OK)
                                            {
                                                 G_TRACE(5)("CLIENT %x -- Written (%d)", client, returned);
                                                if (result != NULL)
                                                {
                                                    MEfree( result );
                                                    result = NULL;
                                                }
                                            }
                                        }
                                    }
                                    while (err == GSTAT_OK && (returned > 0 || moredata));
                                    G_TRACE(5)("CLIENT %x -- Returned page", client);
                                }
                                break;
                        }
                        if (result != NULL)
                        {
                            MEfree( result );
                            result = NULL;
                        }
                    }
                }
            }
            if (params != NULL)
            {
                MEfree( params );
            }
        }
        CLEAR_STATUS( ICEClientCloseConnection(session, (err == GSTAT_OK)));
    }

    TMnow(&stime2);
    G_TRACE(1)("CLIENT %x -- timing %d ms",
                    client,
                    ((stime2.TM_secs - stime1.TM_secs) * 1000) +
                    ((stime2.TM_msecs - stime1.TM_msecs) / 1000));
return(err);
}


/*
** Name: ICEProcess
**
** Description:
**      Main entry for ice client processing.
**
** Inputs:
**      client  client context information.
**
** Outputs:
**      None.
**
** Returns:
**      TRUE    Successfully completed.
**      FALSE   Failed.
**
** History:
**      06-Jun-1999 (fanra01)
**          Add history.
**          Add remote node setup.
**      04-05-2001(wansh01)
**          for dgi_us5, copy string ICE_ERROR_CONTENT to a tempstr before 
**          calling APAPIInitPage. APAPIInitPage is modifying the passed string 
**          which is for read only.  
**      06-28-2001(wansh01)
**          #ifdef tempstr to avoid compiler error for solaris etc platform. 
**      21-Mar-2002 (fanra01)
**          Add re-initialization of apistatus so that server error
**          is eventually returned.
*/
bool
ICEProcess (PICEClient client)
{
    GSTATUS    err = GSTAT_OK;

    /*
    ** if the client node has not previously been set
    ** use the value of the rmt_svr_node.
    ** rmt_svr_node set from the environment variable II_ICE_SERVER_NODE.
    ** If the environment is not set the pointer should be null and local
    ** node is assumed.
    */
    if (client->node == NULL)
    {
        client->node = rmt_svr_node;
    }
    err = ICEQuery(HTML_API, client->node, client);
    if (err != GSTAT_OK)
    {
        GSTATUS    err2 = GSTAT_OK;
        char *page = NULL;
        char *message = ERget(err->number);
#if defined(dgi_us5)
        char tempstr[STlength(ICE_ERROR_CONTENT)+1];
#endif

        err2 = GAlloc(&page,
            STlength(ICE_ERROR_PAGE) + STlength(message) + 50,
            FALSE);
        if (err2 == GSTAT_OK)
        {
            G_TRACE(1)("CLIENT %x -- Error %d (%s)",
                client,
                err->number,
                message);
#if defined(dgi_us5)
               MEcopy(ICE_ERROR_CONTENT,STlength(ICE_ERROR_CONTENT)+1,tempstr);
               err2 = client->initPage (
                tempstr,
                STlength(ICE_ERROR_CONTENT) + 1,
                client);
#else
               err2 = client->initPage (
                ICE_ERROR_CONTENT,
                STlength(ICE_ERROR_CONTENT) + 1,
                client);
#endif
            /*
            ** On error set the api status to zero for failure
            ** to return server error.
            */
            client->apistatus = 0;
            if (err2 == GSTAT_OK)
            {
                STprintf(page, ICE_ERROR_PAGE, message);
                err2 = client->write (
                    STlength(page)+1,
                    page,
                    client);
            }
            MEfree((PTR)page);
            /*
            ** Error connecting with target node.
            ** Attempt to disconnect and remove all saved sessions.
            */
            err = DDFSemOpen (&SessionIntegrity, TRUE);
            if (err == GSTAT_OK)
            {
                while (err == GSTAT_OK && sessionList != NULL)
                {
                    PICEGCASession session;

                    session = sessionList;
                    sessionList = sessionList->next;
                    CLEAR_STATUS(ICEGCADisconnect(session));
                    MEfree((PTR) session);
                }
                CLEAR_STATUS(DDFSemClose(&SessionIntegrity));
            }
        }
        if (err2 != GSTAT_OK)
            DDFStatusFree(TRC_EVERYTIME, &err2);
        DDFStatusFree(TRC_EVERYTIME, &err);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
