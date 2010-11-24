/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/
#include <wsf.h>
#include <wts.h>
#include <erwsf.h>
#include <wss.h>
#include <wpsbuffer.h>
#include <wssapp.h>

#include <ascf.h>
#include <asct.h>

#include "wtslocal.h"

/*
**
**  Name: wts.c - Web Thread Service
**
**  Description:
**		This very important component is in charge of the communication
**		between the Ingres shell and WSF.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	8-Sep-98 (marol01)
**	    fix issue with the upload feature. 
**      11-Sep-1998 (fanra01)
**          Fixup comiler warnings on unix.
**      02-Oct-98 (fanra01)
**          Add determining client lib name from configuration.
**      13-Oct-98 (fanra01)
**          Add testing for empty unit.
**          Incorperate change by marol01 for freeing header after its used.
**      02-Nov-98 (fanra01)
**          Add changes to move file upload to separate module.
**          Add remote address and remote host.
**      16-Nov-98 (fanra01)
**          Add html content string when creating autodeclared session.
**      20-Nov-1998 (fanra01)
**          Add call to free header and data memory areas.
**      18-Jan-1999 (fanra01)
**          Add ascf.h header for ascf function prototypes.  Cleaned up some
**          compiler warnings on unix.
**      15-Feb-1999 (fanra01)
**          Add ICE cleanup function.
**      04-Mar-1999 (fanra01)
**          Add trace messages.
**      17-Aug-1999 (fanra01)
**          Bug 98429
**          Add expiry time for cookie to be returned to the browser.
**      17-Jan-2000 (fanra01)
**          Bug 100035
**          For C API requests the error returned should contain the error
**          number and the error string.
**      25-Jan-2000 (fanra01)
**          Bug 100132
**          Change cookie expiry from using a Netscape extension to the
**          Max-Age defined in RFC 2109.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support. secure and scheme information is extracted from
**          the client request.
**      18-May-2000 (fanra01)
**          Bug 101345
**          Changed the format of respose data sent back from the ice server to
**          the ice client.  Removed the use of GCA_C_FROM message.
**          Moved some of the send logic into ascs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Add setting of content type based on the document extension.
**          Made setting of content type dynamic based on the types set in
**          ACT_SESSION structure.
**      22-Jun-2001 (fanra01)
**          Sir 103096
**          Move setting of content type to both authenticated and
**          non-authenticated sessions.
**      20-Nov-2001 (fanra01)
**          Bug 106434
**          Modified the cookie string returned when no session context is
**          available.  Also correct the string search function to use
**          STstrindex.
**      20-Nov-2001 (fanra01)
**          Correct build of cookie string introduced in previous submission.
**      21-Nov-2001 (fanra01)
**          Bug 106447
**          Change parsing of the cookie string to handle value delimiters.
**          Also scan the list of values in reverse if multiple values are
**          returned from the browser.
**          Bug 106449
**          Add initializations of mimetype pointers.
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**/


static PTR	wts_handle = NULL;
static char     client_lib[MAX_LOC+1] = {0};

GSTATUS WSFInitialize ();
GSTATUS WSFTerminate ();

/*
** Name: WTSOpenPage
**
** Description:
**      Function starts the response message by sending the HTTP header
**      values.
**
** Inputs:
**	scb         server session control block
**	page_type   response type
**	hlength     length of the header buffer
**	header      buffer containing HTTP header.
**
** Outputs:
**      None.
**
** Returns:
**	GSTAT_OK    Successfully completed
**      !0          Failed status
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      18-May-2000 (fanra01)
**          Moved send into ascs where the response is formatted as a number
**          of tuples.
*/
GSTATUS 
WTSOpenPage(
    SCD_SCB *scb,
    u_i4    page_type,
    i4      hlength,
    char    *header )
{
    GSTATUS         err = GSTAT_OK;
    DB_STATUS       status = E_DB_OK;

    status = ascs_format_ice_header( scb, page_type, hlength, header );

    if (status != E_DB_OK)
    {
        err = DDFStatusAlloc( status );
    }
    return(err);
}

/*
** Name: WTSSendPage
**
** Description:
**      Function sends the response data to the client.  Allowing the response
**      to be fragmented.
**
** Inputs:
**	scb         server session control block
**	plength     length of response buffer
**      page        response buffer
**      msgind      beginning or ending buffer.
**
** Outputs:
**      None.
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      18-May-2000 (fanra01)
**          Moved send logic into ascs.  The Send page can be called multiple
**          times to send the complete response.
*/
GSTATUS 
WTSSendPage(
    SCD_SCB *scb,
    i4      plength,
    char*   page,
    i4      msgind,
    i4*     sent )
{
    GSTATUS     err = GSTAT_OK;
    STATUS      status = GSTAT_OK;
    i4          size = 0;
    i4          pwritten = 0;

    if ((status = ascs_format_ice_response( scb, plength, page, msgind, sent ))
        == E_DB_OK)
    {
        if (msgind & ICE_RESP_HEADER)
        {
            status = ascs_gca_flush_outbuf( scb );
        }
    }
    if (status)
    {
        err = DDFStatusAlloc( status );
    }
    return(err);
}

/*
** Name: WTSClosePage
**
** Description:
**      Sends a GCA_RESPONSE message to indicate completed response message.
**
** Inputs:
**    scb           Server session control block.
**
** Outputs:
**    None.
**
** Returns:
**    GSTAT_OK      Successfully completed.
**    !0            Failed status.
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
*/
GSTATUS 
WTSClosePage( SCD_SCB *scb, bool successed)
{
    GSTATUS     err = GSTAT_OK;
    DB_STATUS   status  = E_DB_OK;
    i4          qry_status = GCA_OK_MASK;
    i4          response = TRUE;
    i4          response_type = GCA_RESPONSE;

    if (!successed)
    {
        qry_status = GCA_FAIL_MASK;
        qry_status |= GCA_CONTINUE_MASK;
        qry_status |= GCA_END_QUERY_MASK;
    }

    if (scb->scb_sscb.sscb_interrupt)
        status = ascs_process_interrupt( scb );
    else
        status = ascs_format_response( scb, response_type, qry_status, 1 );

    if (DB_FAILURE_MACRO(status))
            err = DDFStatusAlloc( status );
    else
    {
        status = ascs_gca_send( scb );
        if (status)
            err = DDFStatusAlloc( status );
    }
    ascs_save_tdesc( scb, NULL );

    return(err);
}

/*
** Name: WTSGetStatement() - Get information from GCA
**
** Description:
**
** Inputs:
**	SCD_SCB*	: scb server function
**
** Outputs:
**	u_nat*		: type of the page
**	char**		: message
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WTSGetStatement(
    SCD_SCB *scb,
    i4      *type,
    char*   *message,
    u_i4   *length)
{
    GSTATUS         err = GSTAT_OK;
    GCA_DATA_VALUE  value_hdr;

    G_ASSERT(!ascs_gca_data_available(scb), E_WS0014_NO_INFO_AVAILABLE);

    G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
        scb,
        (PTR)&value_hdr,
        sizeof(value_hdr.gca_type) +
        sizeof(value_hdr.gca_precision) +
        sizeof(value_hdr.gca_l_value))),
        E_WS0013_NO_INFO_RECEIVE);

    err = GAlloc((PTR*)message, *length + value_hdr.gca_l_value + 1, TRUE);
    if (err == GSTAT_OK)
    {
        G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
            scb,
            (PTR)(*message + *length),
            value_hdr.gca_l_value)),
            E_WS0013_NO_INFO_RECEIVE);
        *length += value_hdr.gca_l_value;
        (*message)[*length] = EOS;
        *type = value_hdr.gca_type;
    }
    return(err);
}

/*
** Name: WTSGetData() - Get information from GCA
**
** Description:
**
** Inputs:
**	SCD_SCB*	: scb server function
**
** Outputs:
**	u_nat*		: type of the page
**	char**		: message
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      31-May-2000 (fanra01)
**          Updated to handle blob segments.  Add a more flag to indicate that
**          more blob segments should be read and that the query data header
**          should be excluded.
*/
GSTATUS 
WTSGetData(
    SCD_SCB *scb,
    i4      *type,
    char*   *message,
    u_i4    *length,
    bool    more )
{
    GSTATUS         err = GSTAT_OK;
    GCA_DATA_VALUE  value_hdr;
    ADP_PERIPHERAL  hdr;
    i2              size;
    u_i4            len = *length;
    i4              ind;

    G_ASSERT(!ascs_gca_data_available(scb), E_WS0014_NO_INFO_AVAILABLE);

    if (more == 0)
    {
        G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
            scb,
            (PTR)&value_hdr,
            sizeof(value_hdr.gca_type) +
            sizeof(value_hdr.gca_precision) +
            sizeof(value_hdr.gca_l_value))),
            E_WS0013_NO_INFO_RECEIVE);
            *type = value_hdr.gca_type;
        if (*type == DB_LBYTE_TYPE || *type == DB_GEOM_TYPE   ||
            *type == DB_POINT_TYPE || *type == DB_MPOINT_TYPE ||
            *type == DB_LINE_TYPE  || *type == DB_MLINE_TYPE  ||
            *type == DB_POLY_TYPE  || *type == DB_MPOLY_TYPE  ||
            *type == DB_GEOMC_TYPE )
        {
            G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
                scb,
                (PTR)&hdr,
                ADP_HDR_SIZE )),
                E_WS0013_NO_INFO_RECEIVE);
        }
    }
    if (*type == DB_LBYTE_TYPE || *type == DB_GEOM_TYPE   ||
        *type == DB_POINT_TYPE || *type == DB_MPOINT_TYPE || 
        *type == DB_LINE_TYPE  || *type == DB_MLINE_TYPE  || 
        *type == DB_POLY_TYPE  || *type == DB_MPOLY_TYPE  ||
        *type == DB_GEOMC_TYPE )
    {
        G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
                scb,
                (PTR)&ind,
                sizeof(ind) )),
                E_WS0013_NO_INFO_RECEIVE);
        if (ind == 1)
        {
            G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(scb, (PTR)&size, sizeof(i2))),
                E_WS0013_NO_INFO_RECEIVE);
            err = GAlloc((PTR*)message, *length + size + 1, TRUE);
            if (err == GSTAT_OK)
            {
                (*message)[*length] = EOS;
                G_ASSERT(DB_FAILURE_MACRO(ascs_gca_get(
                    scb,
                    (PTR)(*message + *length),
                    size)),
                    E_WS0013_NO_INFO_RECEIVE);
                (*message)[size + *length] = EOS;
                *length += size;
            }
        }
    }
    return(err);
}

/*
** Name: WTSOpenSession() - Prepare the execution of a client request
**
** Description:
**
** Inputs:
**	SCD_SCB*	: scb server function
**	PTR	*		: web session
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      17-Jan-2000 (fanra01)
**          Set client flag to indicate client type.
**      28-Apr-2000 (fanra01)
**          secure and scheme information is extracted from the client request.
**      12-Jun-2000 (fanra01)
**          Add a moredata flag to WTSGetData call to indicate that there is
**          more query data blob segments to be read.
**      21-Nov-2001 (fanra01)
**          Change the parsing of the cookie string that is passed back from
**          the browser.
*/
#define		GET_STR_PARAMS(X, Y)	\
		if (err == GSTAT_OK && Y != NULL) \
		{ u_i4 length; char buf[STR_LENGTH_SIZE+1]; \
			MECOPY_VAR_MACRO(Y, STR_LENGTH_SIZE, buf); \
		  buf[STR_LENGTH_SIZE] = EOS; Y += STR_LENGTH_SIZE;\
 		  if (CVan(buf, (i4 *)&length) == OK && length > 0) \
			{	err = GAlloc((PTR*)&X, length + 1, FALSE);\
			if (err == GSTAT_OK) { MECOPY_VAR_MACRO(Y, length, X);\
			X[length] = EOS; }\
			} \
			Y += length;\
		}

#define		GET_NUM_PARAMS(X, Y) \
		if (err == GSTAT_OK && Y != NULL) \
		{ char buf[NUM_MAX_INT_STR+1]; MECOPY_VAR_MACRO(Y, NUM_MAX_INT_STR, buf);\
		buf[NUM_MAX_INT_STR] = EOS; CVan(buf, (i4*)&X); Y += NUM_MAX_INT_STR;}

GSTATUS
WTSOpenSession(
    SCD_SCB     *scb,
    WTS_SESSION *session)
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = OK;
    u_i4   read = 0;
    u_i4   client;
    i4      type;
    i4 dataSize;
    char    *query = NULL;
    char    *content = NULL;
    char    *path = NULL;
    char    *cookie = NULL;
    char    *port = NULL;
    char    *variable = NULL;
    char    *auth_type = NULL;
    char    *host = NULL;
    char    *agent = NULL;
    char    *rmt_addr = NULL;
    char    *rmt_host = NULL;
    char    *secure = NULL;
    char    *scheme = NULL;

    err = WTSGetStatement(scb, &type, &query, &read);
    if (err == GSTAT_OK)
    {
        if (type != DB_QTXT_TYPE)
            err = DDFStatusAlloc(E_WS0014_NO_INFO_AVAILABLE);
        else
        {
            char *buffer = query;
            i4 total = 0;

            GET_NUM_PARAMS(client, buffer);
            GET_STR_PARAMS(content, buffer);
            GET_STR_PARAMS(path, buffer);
            GET_STR_PARAMS(host, buffer);
            GET_STR_PARAMS(port, buffer);
            GET_STR_PARAMS(cookie, buffer);
            GET_STR_PARAMS(auth_type, buffer);
            GET_STR_PARAMS(session->gateway, buffer);
            GET_STR_PARAMS(agent, buffer);
            GET_STR_PARAMS(rmt_addr, buffer);
            GET_STR_PARAMS(rmt_host, buffer);
            GET_STR_PARAMS(secure, buffer);
            GET_STR_PARAMS(scheme, buffer);
            GET_NUM_PARAMS(dataSize, buffer);
            if (err == GSTAT_OK && dataSize > 0)
            {
                i4 available = 0;
                bool moredata = FALSE;

                session->client = client;
                read = 0;
                err = WTSOpenUpLoad(content, session);
                if (err == GSTAT_OK)
                {
                    while (err == GSTAT_OK && dataSize > total)
                    {
                        err = WTSGetData(scb, &type, &query, &read, moredata);
                        if (err == GSTAT_OK)
                        {
                            if (type != DB_LBYTE_TYPE && type != DB_GEOM_TYPE &&
                              type != DB_POINT_TYPE && type != DB_MPOINT_TYPE &&
                              type != DB_LINE_TYPE  && type != DB_MLINE_TYPE  &&
                              type != DB_POLY_TYPE  && type != DB_MPOLY_TYPE  &&
                              type != DB_GEOMC_TYPE )
                                err = DDFStatusAlloc(E_WS0014_NO_INFO_AVAILABLE);
                            else if (session->status != NO_UPLOAD)
                            {
                                total += read;
                                available = read;
                                err = WTSUpLoad(query, &available, session);
                                if (available > 0)
                                {
                                    MECOPY_VAR_MACRO(query + (read-available),
                                        available, query);
                                    total -= available;
                                }
                                read = available;
                                moredata = (dataSize > total);
                            }
                            else
                                total = read;
                        }
                    }

                    if (err == GSTAT_OK)
                    if (session->status != NO_UPLOAD)
                    {
                        variable = session->variable;
                        session->variable = NULL;
                    }
                    else
                    {
                        variable = query;
                        query = NULL;
                    }
                    CLEAR_STATUS(WTSCloseUpLoad(session));
                }
            }
        }
    }
    if (err == GSTAT_OK)
    {
        char*           tmp = NULL;
        char*           value = NULL;
        USR_PSESSION    usr_session = NULL;

        /*
        ** Use STstrindex instead of STindex to find string-in-string.
        */
        tmp = STrstrindex( cookie, HVAR_COOKIE, 0, FALSE );
        while(tmp != NULL)
        {
            char *p = tmp + STlength(HVAR_COOKIE);

            /*
            ** Scan for the value
            */
            for (; *p != EOS && *p != '=' && *p != ';'; p += 1);
            switch(*p)
            {
                case '=':
                    p += 1;     /* skip the '=' character */
                    value = p;  /* get the value */
                    /*
                    ** scan for the end
                    */
                    for (; *p != EOS && *p != ';' && *p != ' '; p +=1 );
                    *p = EOS;
                    if (*value != EOS)
                    {
                        err = WSMRequestUsrSession(value,
                            &usr_session);
                        if (err != GSTAT_OK && usr_session == NULL)
                        {
                            switch(err->number)
                            {
                                case E_WS0011_WSS_NO_OPEN_SESSION:
                                case E_WS0077_WSS_TIMEOUT:
                                    if ((client & C_API) == 0)
                                    {
                                        if (tmp != cookie)
                                        {
                                            tmp = STrstrindex(cookie, HVAR_COOKIE,
                                                tmp - cookie + 1, FALSE);
                                        }
                                        else
                                        {
                                            tmp = NULL;
                                        }
                                        DDFStatusFree( TRC_INFORMATION, &err );
                                    }
                                    break;
                                default:
                                    tmp = NULL;
                                    break;
                            }
                        }
                        else
                        {
                            client |= CONNECTED;
                            tmp = NULL;
                        }
                    }
                    else
                    {
                        if (tmp != cookie)
                        {
                            tmp = STrstrindex(cookie, HVAR_COOKIE,
                                tmp - cookie + 1, FALSE);
                        }
                        else
                        {
                            tmp = NULL;
                        }
                    }
                    break;
                case ';':
                case EOS:
                    /*
                    ** no value see if there's another
                    */
                    if (tmp != cookie)
                    {
                        tmp = STrstrindex(cookie, HVAR_COOKIE,
                            tmp - cookie + 1, FALSE);
                    }
                    else
                    {
                        tmp = NULL;
                    }
                    break;
            }
        }
        if (err == GSTAT_OK)
        {
            err = WSSOpenSession (
                client,
                path,
                secure,
                scheme,
                host,
                port,
                variable,
                auth_type,
                session->gateway,
                agent,
                rmt_addr,
                rmt_host,
                usr_session,
                &session->act_session);
        }
    }

    MEfree(agent);
    MEfree(rmt_addr);
    MEfree(rmt_host);
    MEfree(secure);
    MEfree(scheme);
    MEfree(query);
    MEfree(host);
    MEfree(cookie);
    MEfree(variable);
    MEfree(auth_type);
    MEfree(content);
    MEfree(path);
    MEfree(port);
    return(err);
}

/*
** Name: WTSCloseSession() - clean the client request
**
** Description:
**
** Inputs:
**	SCD_SCB*	: scb server function
**	PTR	*		: web session
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WTSCloseSession(
	SCD_SCB			*scb, 
	WTS_SESSION	*session,
	bool				force) 
{
	GSTATUS err = GSTAT_OK;

	if (session != NULL)
	{
		UPLOADED_FILE *tmp;

		while (session->list != NULL)
		{
			tmp = session->list;
			session->list = session->list->next;
			CLEAR_STATUS(WCSRelease(&tmp->file));
			MEfree((PTR)tmp);
		}
		if (session->act_session != NULL)
			CLEAR_STATUS(WSSCloseSession(&session->act_session, force));
	}
return(err);
}

/*
** Name: WTSInitialize() - Set the Web Server Facility
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      02-Oct-98 (fanra01)
**          Add obtaining client library name from config.
*/
DB_STATUS
WTSInitialize (DB_ERROR *err)
{
    DB_STATUS dbstatus = E_DB_ERROR;
    GSTATUS status = GSTAT_OK;
    char* client;

    status = WTSUploadInit();

    if (status == GSTAT_OK)
        status = WSFInitialize();

    if (status == GSTAT_OK)
    {
        if ((PMget( CONF_CLIENT_LIB, &client ) != OK) &&
            (client == NULL))
        {
            status = DDFStatusAlloc(E_WS0092_WCS_UNDEF_CLIENT_LIB);
        }
        else
        {
            STcopy (client, client_lib);
        }

        err->err_code = ((status == GSTAT_OK) ? E_DB_OK : status->number);
        dbstatus = err->err_code;
    }
    else
    {
        err->err_code = status->number;
    }
    if (status != GSTAT_OK)
    {
        DDFStatusFree(TRC_EVERYTIME, &status);
    }
    return(dbstatus);
}

/*
** Name: WTSTerminate() - Clean the Web Server Facility
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
DB_STATUS
WTSTerminate (DB_ERROR *err)
{
	GSTATUS status = GSTAT_OK;
	status = WSFTerminate();

	if (status == GSTAT_OK)
	{
		err->err_code = E_DB_OK;
		return(E_DB_OK);
	}
	else
	{
		err->err_code = status->number;	
		DDFStatusFree(TRC_EVERYTIME, &status);
		return(E_DB_ERROR);
	}
}

/*
** Name: WTSExecute
**
** Descripition:
**
**
**
** Input:
**      scb         session control block
**
** Output:
**      none.
**
** Return:
**      E_DB_OK     completed sucessfully
**      E_DB_ERROR  failed
**
** History:
**      29-May-98 (fanra01)
**          Integrated from ice for marol01.
**      13-Oct-98 (fanra01)
**          Add testing for empty unit.
**          Incorperate change by marol01 for freeing header after its used.
**      16-Nov-98 (fanra01)
**          Add html content string when creating autodeclared session.
**      20-Nov-1998 (fanra01)
**          Add call to free header and data memory areas.
**      17-Aug-1999 (fanra01)
**          Add expiry time for cookie to be returned to the browser.
**      17-Jan-2000 (fanra01)
**          Change the return for C API so that the error number is returned.
**          Also the web wrapper message is not needed for C API.
**      25-Jan-2000 (fanra01)
**          Bug 100132
**          Change the cookie attribute for timeout from Netscape absolute
**          time to a delta time in seconds.
**      18-May-2000 (fanra01)
**          Add passing of msgind flag to WTSOpenPage.
**      11-Oct-2000 (fanra01)
**          Add setting of content type based on the document extension.
**          Made setting of content type dynamic based on the types set in
**          ACT_SESSION structure.
**      21-Nov-2001 (fanra01)
**          Bug 106449
**          Add initialization of mimebase and mimeext pointers as they
**          could be random and return an unknown mimetype causing the
**          browser to prompt for download.
*/
DB_STATUS
WTSExecute (SCD_SCB *scb)
{
    GSTATUS         err = GSTAT_OK;
    WTS_SESSION     session;
    WPS_BUFFER      buffer;
    char*           application = NULL;
    char*           page = NULL;
    char*           header = NULL;
    bool            kill = TRUE;
    char*           mimebase = NULL;
    char*           mimeext  = NULL;
    i4              mimelen = 0;

    MEfill (sizeof(WTS_SESSION), 0, (PTR)&session);
    MEfill (sizeof(WPS_BUFFER), 0, (PTR)&buffer);

    err = WPSHeaderInitialize(&buffer);
    if (err == GSTAT_OK)
        err = WPSBlockInitialize(&buffer);
    if (err == GSTAT_OK)
        err = WTSOpenSession(scb, &session);

    if (err == GSTAT_OK &&
        session.act_session != NULL &&
        session.act_session->query != NULL)
    {
        asct_trace( ASCT_LOG )( ASCT_TRACE_PARAMS,
            "Request session=%s query=%s",
             ((session.act_session->user_session) ?
                 session.act_session->user_session->name : ""),
             session.act_session->query );

        page = session.act_session->query;
        application = session.act_session->query;

        if (application[0] == CHAR_URL_SEPARATOR)
        {
            page++;
            while (page[0] != EOS &&
                page[0] != CHAR_URL_SEPARATOR &&
                page[0] != WSS_UNIT_BEGIN)
                page++;
            if (page[0] == CHAR_URL_SEPARATOR)
            {
                bool result;
                page[0] = EOS;
                CLEAR_STATUS(WSSAppExist (application + 1, &result));
                if (err == GSTAT_OK && result == FALSE)
                {
                    page[0] = CHAR_URL_SEPARATOR;
                    application = NULL;
                    page = session.act_session->query;
                }
                else
                    page++;
            }
            else
            {
                i4    unitlen;
                switch (*page)
                {
                    case WSS_UNIT_BEGIN:
                        for (unitlen=0, page++; (*page != EOS);
                            page++, unitlen++)
                        {
                            if (*page == WSS_UNIT_END)
                            {
                                break;
                            }
                        }
                        if (unitlen && *page == WSS_UNIT_END)
                        {
                            page = session.act_session->query;
                            application = NULL;
                        }
                        else
                        {
                            err = DDFStatusAlloc(E_WS0093_WTS_UNDEF_UNIT_PAGE);
                        }
                        break;

                    case CHAR_URL_SEPARATOR:
                    default:
                        page = session.act_session->query;
                        application = NULL;
                        break;
                }
            }
        }
    }

    if (application == NULL || application[0] != CHAR_URL_SEPARATOR)
        application = "/";

    if (err == GSTAT_OK)
    {

        kill = FALSE;
        err = WSSSetBuffer(session.act_session, &buffer, page);

        mimebase = session.act_session->mimebase;
        mimeext  = session.act_session->mimeext;
        mimelen = ((mimebase != NULL) ? STlength( mimebase ) : sizeof(STR_TYPE_BASE)) +
            ((mimeext != NULL) ? STlength( mimeext ) : sizeof(STR_HTML_EXT));

        if (session.act_session->user_session != NULL &&
            session.act_session->type != WSM_DISCONNECT)
        {
            char  expire[30];

            MEfill( sizeof(expire), 0, expire );
            WSMGetCookieMaxAge( session.act_session->user_session, expire );

            CLEAR_STATUS(G_ME_REQ_MEM(
                0, header, char,
                STlength(STR_COOKIE) +
                STlength(STR_NEWLINE) +
                STlength(STR_COOKIE_MAXAGE) +
                STlength(HVAR_COOKIE) +
                STlength(HTTP_CONTENT) +
                mimelen +
                STlength(client_lib) +
                STlength(session.act_session->user_session->name) +
                STlength(session.gateway) +
                STlength(application) +
                STlength(expire) + 50));
            if (header != NULL)
            {
                /*
                ** If the name contains a value set the cookie string to be
                ** content-type: text/html
                ** Set cookie: ii_cookie=value; path=value; ...
                ** otherwise set the cookie string to be
                ** content-type: text/html
                ** Set cookie: ii_cookie; path=value; ...
                */
                if (session.act_session->user_session->name != NULL)
                {
                    STprintf ( header,
                        HTTP_CONTENT STR_COOKIE STR_SPACE STR_COOKIE_MAXAGE STR_NEWLINE,
                    (mimebase != NULL) ? mimebase : STR_TYPE_BASE,
                    (mimeext != NULL) ? mimeext : STR_HTML_EXT,
                    HVAR_COOKIE,
                    STR_EQUALS_SIGN,
                    session.act_session->user_session->name,
                    client_lib,
                    session.gateway,
                    application,
                    expire );
                }
                else
                {
                    STprintf ( header,
                        HTTP_CONTENT STR_COOKIE STR_SPACE STR_COOKIE_MAXAGE STR_NEWLINE,
                    (mimebase != NULL) ? mimebase : STR_TYPE_BASE,
                    (mimeext != NULL) ? mimeext : STR_HTML_EXT,
                    HVAR_COOKIE,
                    STR_EMPTY,
                    STR_EMPTY,
                    client_lib,
                    session.gateway,
                    application,
                    expire );
                }
            }
        }
    }

    if (header == NULL)
    {
        CLEAR_STATUS(G_ME_REQ_MEM(
            0, header, char,
            STlength(STR_COOKIE) +
            STlength(STR_NEWLINE) +
            STlength(HVAR_COOKIE) +
            STlength(HTTP_CONTENT) +
            mimelen +
            STlength(client_lib) +
            STlength(session.gateway) +
            STlength(application) + 50));
        /*
        ** If there is no ICE session context return an empty cookie string 
        ** content-type: text/html
        ** Set cookie: ii_cookie;
        */
        if (header != NULL)
            STprintf ( header,
                HTTP_CONTENT STR_COOKIE STR_NEWLINE,
                (mimebase != NULL) ? mimebase : STR_TYPE_BASE,
                (mimeext != NULL) ? mimeext : STR_HTML_EXT,
                HVAR_COOKIE,
                STR_EMPTY,
                STR_EMPTY,
                client_lib,
                session.gateway,
                application);
    }
    if (mimebase)
    {
        MEfree(mimebase);
    }
    if (mimeext)
    {
        MEfree(mimeext);
    }
    CLEAR_STATUS(WTSCloseSession(scb, &session, kill));

    if (header != NULL)
    {
        CLEAR_STATUS(WPSHeaderAppend (&buffer, header, STlength(header)));
        MEfree ((PTR)header);
    }
    if (err != GSTAT_OK)
    {
        GSTATUS err1 = GSTAT_OK;
        char *message = NULL;
        char *text = NULL;
        char *tmp = NULL;
        u_i4 client = session.client;

        WPS_BUFFER_EMPTY_BLOCK(&buffer);
        err1 = G_ST_ALLOC(message, ERget(err->number));
        if (err1 == GSTAT_OK)
        {
            text = ERget(I_WS0000_WSF_ERROR);
            /*
            ** Add adjustment for C_API client return status.
            */
            err1 = G_ME_REQ_MEM (
                0,
                tmp,
                char,
                ((client & C_API) ? (NUM_MAX_INT_STR + STR_LENGTH_SIZE) : 0) +
                ((text) ? STlength(text) : 0) +
                ((message) ? STlength(message) : 0) +
                ((err->info) ? STlength(err->info) : 0) + 1);
            if (err1 == GSTAT_OK)
            {
                /*
                ** If the client is api the error status must be returned as a
                ** value.
                */
                if (client & C_API)
                {
                    STprintf(tmp, "%*d", NUM_MAX_INT_STR, err->number);
                    STprintf( tmp, "%s%*d%s", tmp, STR_LENGTH_SIZE,
                        STlength( message ), message );
                }
                else
                {
                    STprintf(tmp, text,
                        (message) ? message : "",
                        (err->info) ? err->info : "");
                }
                err1 = WPSBlockAppend (&buffer, tmp, STlength(tmp));
            }
        }
        if (err1 == GSTAT_OK)
            DDFStatusFree(TRC_EVERYTIME, &err1);
        MEfree((PTR)message);
        MEfree((PTR)tmp);
        DDFStatusFree(TRC_EVERYTIME, &err);
    }

    err = WTSOpenPage(scb,
        buffer.page_type,
        buffer.header_position,
        buffer.header);

    if (err == GSTAT_OK)
    {
        char*   tmp = NULL;
        i4      block_size = 0;
        i4      sent = 0;
        i4      total = 0;
        u_i4    current = 0;
        u_i4    read = 0;
        i4      buf_length = 0;
        i4      msgind = ICE_RESP_HEADER;
        i4      bufleft;
        i4      written = 0;
        i4      totwrite = 0;

        do
        {
            err = WPSBufferGet(&buffer, &read, &total, &tmp, buf_length);
            if (err == GSTAT_OK && read > 0)
            {
                totwrite = (totwrite == 0) ? total : totwrite;
                buf_length += read;
                do
                {
                    bufleft = DB_MAXTUP - ascs_tuple_len( scb );
                    bufleft -= sizeof(i4) + sizeof(i2);
                    bufleft -= (msgind & ICE_RESP_HEADER) ? ADP_HDR_SIZE : 0;
                    /*
                    ** if remaining buffer (including a terminating
                    ** indicator will fit into the tuple buffer then make this
                    ** the last message
                    */
                    if (((totwrite - (written + buf_length)) == 0) &&
                        (buf_length <= (i4)(bufleft - sizeof(i4))))
                    {
                        msgind |= ICE_RESP_FINAL;
                    }
                    err = WTSSendPage( scb, buf_length, tmp, msgind, &sent );
                    msgind &= ~ICE_RESP_HEADER; /* turn off header flag */
                    buf_length -= sent;
                    tmp += sent;
                    written += sent;
                }
                while(buf_length > 0);

            }
        }
        while (err == GSTAT_OK && read > 0);

        if (err == GSTAT_OK && buf_length > 0)
            err = WTSSendPage(scb, buf_length, tmp, ICE_RESP_FINAL, &sent);

        WPSFreeBuffer (&buffer);

        CLEAR_STATUS(WTSClosePage(scb, (err == GSTAT_OK)));
    }
    MEfree((PTR)session.gateway);
    DDFStatusFree(TRC_EVERYTIME, &err);
    return (E_DB_OK);
}

/*
** Name: WTSCleanup
**
** Description:
**      Function to be called periodically from the the cleanup thread.
**      This ensures that any timed out resources are removed.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      E_DB_OK     completed sucessfully
**      E_DB_ERROR  failed
**
** History:
**      04-Feb-1999 (fanra01)
**          Created.
*/
DB_STATUS
WTSCleanup (DB_ERROR *dberr)
{
    GSTATUS status = GSTAT_OK;

    if ((status = WSMCleanUserTimeout()) == GSTAT_OK)
    {
        if ((status = WPSCleanFilesTimeout(NULL)) == GSTAT_OK)
        {
            if ((status = DBCleanTimeout()) == GSTAT_OK)
            {
                dberr->err_code = E_DB_OK;
                return(E_DB_OK);
            }
        }
    }

    if (status != GSTAT_OK)
    {
        dberr->err_code = status->number;
        DDFStatusFree(TRC_EVERYTIME, &status);
        return(E_DB_ERROR);
    }
    return(E_DB_OK);
}
