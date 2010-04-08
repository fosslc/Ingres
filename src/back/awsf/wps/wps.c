/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wps.h>
#include <htmlmeth.h>
#include <htmlpars.h>
#include <erwsf.h>
#include <wpspost.h>
#include <wcs.h>
#include <wsmvar.h>

/*
**
**  Name: wps.c - Html parser for ICE macro command
**
**  Description:
**	that file provides functions to parse and execute ICE macro command files
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      05-Oct-1998 (fanra01)
**          Changed page return type to WPS_URL_BLOCK.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Removed setting of content type as is done in the WTS function
**          when writing the HTTP header.
**      20-Jun-2001 (fanra01)
**          Sir 103096
**          Add specifing of certain output formats from HTML variable.
**      22-Jul-2002 (fanra01)
**          Bug 108329
**          Ensure order for error messages when processing from variables
**          input.
**      20-Aug-2003 (fanra01)
**          Bug 110747
**          Additional output information on encountering a parser error. 
**/

GLOBALREF i4  max_memory_block;
GLOBALREF i4  block_unit_size;
GLOBALREF	i4	ingresII_driver;

/*
** Name: WPSPrintError() - Format into the returned HTML page the error generated 
**						   the one of the macros located into the source file
**
** Description:
**
** Inputs:
**	ACT_PSESSION : act_session
**	u_i4		 : number
**	char*		 : text
**	u_i4		 : length
**	WPS_PBUFFER	 : buffer
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
WPSPrintError (
	ACT_PSESSION act_session, 
	u_i4		 number,
	WPS_PBUFFER	 buffer,
	WPS_PBUFFER  result) 
{
  GSTATUS err = GSTAT_OK;
	WPS_PBUFFER tmp = NULL;
	CL_ERR_DESC clError;
	i4 nMsgLen;	
	char szMsg[ERROR_BLOCK];

	ERslookup (
			number, 
			NULL, 
			0, 
			NULL, 
			szMsg, 
			ERROR_BLOCK,
			-1, 
			&nMsgLen, 
			&clError, 
			0, 
			NULL);
	szMsg[nMsgLen] = EOS;

	err = G_ME_REQ_MEM(0, tmp, WPS_BUFFER, 1);
	if (err == GSTAT_OK)
	{
		err = WPSBlockInitialize(tmp);
		if (err == GSTAT_OK)
		{
			err = WPSBlockAppend(tmp, TAG_HTML, STlength(TAG_HTML));
			if (err == GSTAT_OK)
				err = WPSBlockAppend(tmp, TAG_BODY, STlength(TAG_BODY));

			if (err == GSTAT_OK)
				err = WPSBlockAppend(tmp, szMsg, nMsgLen);

			if (err == GSTAT_OK)
				err = WPSPlainText(tmp, buffer);

			if (err == GSTAT_OK)
				err = WPSBlockAppend(tmp, TAG_END_BODY, STlength(TAG_END_BODY));

			if (err == GSTAT_OK)
				err = WPSBlockAppend(tmp, TAG_END_HTML, STlength(TAG_END_HTML));

			if (err == GSTAT_OK)
			{
				err = WPSBlockInitialize(result);
				if (err == GSTAT_OK)
					err = WPSMoveBlock (result, tmp); 
			}
			CLEAR_STATUS( WPSFreeBuffer(tmp) );
		}
		MEfree((PTR)tmp);
	}
return(err);
}

/*
** Name: WPSPerformMacro() - Macro execution entry point
**
** Description:
**	Open the macro command file.
**	parse and Execute it
**	format success or error
**
** Inputs:
**	ACT_PSESSION : active session
**	WPS_FILE*	 : macro command file
**
** Outputs:
**	WPS_PBUFFER	 : result buffer
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
**      28-Oct-98 (fanra01)
**          Add code to handle a cached file of > 4K.  In this instance the
**          buffer is copied to the result area allowing further reads into
**          the read area.
**      11-Oct-2000 (fanra01)
**          Removed setting of content type as is done in the WTS function
**          when writing the HTTP header.
**      20-Aug-2003 (fanra01)
**          Add path and filename to error message if parser error occurs.
*/

GSTATUS
WPSPerformMacro (
    ACT_PSESSION    act_session,
    WPS_PBUFFER     buffer,
    WPS_FILE        *cache)
{
    GSTATUS err = GSTAT_OK;

    err = WCSOpen (cache->file, "r", SI_BIN, 0);

    if (err == GSTAT_OK)
    {
        char *tmp;

        err = G_ME_REQ_MEM(0, tmp, char, WSF_READING_BLOCK);
        if (err == GSTAT_OK)
        {
            PARSER_IN in;
            WPS_HTML_PPARSER_OUT out;

            MEfill (sizeof (PARSER_IN), 0, (PTR)&in);
            in.first_node = HTML_FIRST_NODE;
            in.buffer = tmp;
            in.size = WSF_READING_BLOCK;

            err = G_ME_REQ_MEM(0, out, WPS_HTML_PARSER_OUT, 1);
            if (err == GSTAT_OK)
            {
                i4 read = 0;
                out->act_session = act_session;
                out->result = buffer;
                err    = WPSBlockInitialize(&out->buffer);

                if (err == GSTAT_OK)
                    err = ParseOpen(html, &in, (PPARSER_OUT) out);

                if (err == GSTAT_OK)
                {
                    do
                    {
                        err = WCSRead(cache->file,
                            WSF_READING_BLOCK - in.length, &read,
                            tmp + in.length);
                        if (err == GSTAT_OK && read > 0)
                        {
                            in.length += read;
                            if (err == GSTAT_OK)
                            err = Parse(html, &in, (PPARSER_OUT) out);

                            if (err == GSTAT_OK)
                            {
                                /*
                                ** If there is a full buffer force a copy
                                ** to the result buffer.
                                */
                                if (in.position == 0)
                                {
                                    err = WPSBlockAppend(out->result, tmp,
                                        in.length);
                                    in.length = 0;
                                    in.current = 0;
                                }
                                else
                                {
                                    MECOPY_VAR_MACRO(tmp + in.position,
                                        in.length - in.position, tmp);
                                    in.length -= in.position;
                                    in.current = in.current - in.position;
                                }
                                in.position = 0;
                            }
                        }
                    }
                    while (err == GSTAT_OK && read > 0);
                }
                if (err == GSTAT_OK)
                    err = ParseClose(html, &in, (PPARSER_OUT) out);

                if (err != GSTAT_OK)
                {
                    DDFStatusInfo( err, "Macro file = %s%c%s\n", 
                        cache->file->info->path, CHAR_PATH_SEPARATOR,
                        cache->file->info->name);
                }
                if (err == GSTAT_OK)
                {
                    if (act_session->error_counter > 0)
                    err = WPSPrintError(
                                act_session,
                                I_WS0057_WPS_MACRO_EXEC,
                                buffer,
                                buffer);
                }
                else
                {
                    GSTATUS status = err;
                    err = WPSPrintError(
                            act_session,
                            status->number,
                            buffer,
                            buffer);
                    DDFStatusFree( TRC_INFORMATION, &status );
                }

                CLEAR_STATUS( WPSFreeBuffer(&out->buffer) );
                MEfree((PTR)out);
            }
            MEfree((PTR)tmp);
        }
        CLEAR_STATUS( WCSClose(cache->file) );
    }

    return(err);
}

/*
** Name: WPSPerformVariable() - entry point for posted execution
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**
** Outputs:
**	WPS_PBUFFER	 : result buffer
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
**      05-Oct-1998 (fanra01)
**          Changed page return type to WPS_URL_BLOCK.  Update execute
**          conditions.
**      20-Jun-2001 (fanra01)
**          Add passing of value from ii_rel_type variable to WPSExecuteQuery.
**      21-Sept-2001 (zhahu02)
**          If WPSExecuteProcedure is successful, then set
**             err = GSTAT_OK. (b105852)
**      22-Jul-2002 (fanra01)
**          Back out change for bug 105852 to avoid exception when no success
**          or error url are defined.
*/
GSTATUS 
WPSPerformVariable (
    ACT_PSESSION    act_session,
    WPS_PBUFFER        buffer)
{
    GSTATUS err    = GSTAT_OK;
    char*   value = NULL;
    char*   reltype = NULL;
    char*   urlError = NULL;
    char*   urlSuccess = NULL;
    char*   msgError = NULL;
    char*   msgSuccess = NULL;
    bool    usrError = FALSE;
    bool    usrSuccess = FALSE;
    i4      outtype = HTML_ICE_TYPE_TABLE;

    err = WSMGetVariable (
        act_session, 
        HVAR_SUCCESS_URL, 
        STlength(HVAR_SUCCESS_URL), 
        &urlSuccess, 
        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    if (err == GSTAT_OK && urlSuccess == NULL)
        err = WSMGetVariable (
            act_session,
            HVAR_SUCCESS_MSG,
            STlength(HVAR_SUCCESS_MSG),
            &msgSuccess,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    if (err == GSTAT_OK)
        err = WSMGetVariable (
            act_session,
            HVAR_ERROR_URL,
            STlength(HVAR_ERROR_URL),
            &urlError,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    if (err == GSTAT_OK && urlError == NULL)
        err = WSMGetVariable (
            act_session,
            HVAR_ERROR_MSG,
            STlength(HVAR_ERROR_MSG),
            &msgError,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    usrSuccess = (urlSuccess != NULL && urlSuccess[0] != EOS) ||
                 (msgSuccess != NULL && msgSuccess[0] != EOS);
    usrError = (urlError != NULL && urlError[0] != EOS) ||
               (msgError != NULL && msgError[0] != EOS);

    if (err == GSTAT_OK)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_PROCEDURE_NAME,
            STlength(HVAR_PROCEDURE_NAME),
            &value,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK && value != NULL)
        {
            err = WPSExecuteProcedure (
                usrSuccess,
                usrError,
                act_session,
                buffer,
                value);
	}
    }
    if (err == GSTAT_OK && value == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_QUERY_STATEMENT,
            STlength(HVAR_QUERY_STATEMENT),
            &value,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK && value != NULL)
        {
            /*
            ** Add test of value from ii_rel_type variable
            */
            err = WSMGetVariable (
                act_session,
                HVAR_RELATION_TYPE,
                STlength(HVAR_RELATION_TYPE),
                &reltype,
                WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
            if (err == GSTAT_OK && reltype != NULL)
            {
                if (STbcompare( STR_RELATION_HTML, 0, reltype, 0, TRUE ) == 0)
                {
                    outtype = HTML_ICE_TYPE_TABLE;
                }
                else if (STbcompare( STR_RELATION_XML, 0, reltype, 0, TRUE ) == 0)
                {
                    outtype = XML_ICE_TYPE_XML;
                }
                else if (STbcompare( STR_RELATION_XMLPDATA, 0, reltype, 0, TRUE ) == 0)
                {
                    outtype = XML_ICE_TYPE_XML_PDATA;
                }
                else if (STbcompare( STR_RELATION_RAW, 0, reltype, 0, TRUE ) == 0)
                {
                    outtype = HTML_ICE_TYPE_RAW;
                }
            }
            err = WPSExecuteQuery (
                usrSuccess,
                usrError,
                act_session,
                buffer,
                value,
                outtype);
        }
    }

    if (err == GSTAT_OK && value == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_REPORT_NAME,
            STlength(HVAR_REPORT_NAME),
            &value,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK  && value != NULL)
            err = WPSExecuteReport (
                usrSuccess,
                usrError,
                act_session,
                buffer,
                value,
                NULL);
    }

    if (err == GSTAT_OK && value == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_REPORT_LOCATION,
            STlength(HVAR_REPORT_LOCATION),
            &value,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK  && value != NULL)
            err = WPSExecuteReport (
                usrSuccess,
                usrError,
                act_session, 
                buffer,
                NULL,
                value);
    }

    if (err == GSTAT_OK && value == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_APPLICATION,
            STlength(HVAR_APPLICATION),
            &value,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK  && value != NULL)
            err = WPSExecuteApp (
                usrSuccess,
                usrError,
                act_session,
                buffer,
                value);
    }

    if (err == GSTAT_OK && value == NULL)
        err = DDFStatusAlloc(E_WS0017_IM_LOST);

    if ((err == GSTAT_OK) || (err->number == E_WS0031_PROCEXEC_OK))
    {
        if ((urlSuccess == NULL) || (*urlSuccess == 0))
        {
            buffer->page_type = WPS_HTML_BLOCK;
            if (msgSuccess != NULL)
                err = WPSBlockAppend(
                    buffer,
                    msgSuccess,
                    STlength(msgSuccess));
            else
                if (buffer->position == 0)
                {
                    CL_ERR_DESC clError;
                    i4  nMsgLen;
                    char szMsg[ERROR_BLOCK];

                    ERslookup (
                        E_WS0027_DEFAULT_SUCCESS, 
                        NULL, 
                        0, 
                        NULL, 
                        szMsg, 
                        ERROR_BLOCK,
                        -1, 
                        &nMsgLen, 
                        &clError, 
                        0, 
                        NULL);
                    szMsg[nMsgLen] = EOS;

                    err = WPSBlockAppend(buffer, szMsg, nMsgLen);
                }
        }
        else
        {
            err = WPSBlockInitialize(buffer);
            buffer->page_type = WPS_URL_BLOCK;

            if (err == GSTAT_OK)
                err = WPSBlockAppend(buffer, 
                    urlSuccess, 
                    STlength(urlSuccess));
        }
    }
    else
    {
        if ((urlError == NULL) || (*urlError == 0))
        {
            buffer->page_type = WPS_HTML_BLOCK;
            if (msgError != NULL)
                err = WPSBlockAppend(buffer, msgError, STlength(msgError));
            else
                if (buffer->position == 0)
                {
                    CL_ERR_DESC clError;
                    i4  nMsgLen;
                    char szMsg[ERROR_BLOCK];

                    ERslookup (
                        E_WS0028_DEFAULT_ERROR, 
                        NULL, 
                        0, 
                        NULL, 
                        szMsg, 
                        ERROR_BLOCK,
                        -1, 
                        &nMsgLen, 
                        &clError, 
                        0, 
                        NULL);
                    szMsg[nMsgLen] = EOS;

                    err = WPSBlockAppend(buffer, szMsg, nMsgLen);
                }
        }
        else
        {
            err = WPSBlockInitialize(buffer);
            buffer->page_type = WPS_URL_BLOCK;

            if (err == GSTAT_OK)
                err = WPSBlockAppend(buffer, urlError, STlength(urlError));
        }
        DDFStatusFree(TRC_EVERYTIME, &err);
    }
    return(err);
}


/*
** Name: WPSInitialize() - 
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
**      06-Jul-98 (fanra01)
**          Add file information ima object.
*/
GSTATUS 
WPSInitialize () 
{
	GSTATUS err = GSTAT_OK;
	char	szConfig[CONF_STR_SIZE];
	char*	size = NULL;

	STprintf (szConfig, CONF_WPS_BLOCK_MAX, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &max_memory_block);

	STprintf (szConfig, CONF_WPS_BLOCK_SIZE, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &block_unit_size);

	if (err == GSTAT_OK)
		err = WPSFileInitialize();
return(err);
}

/*
** Name: WPSTerminate() - 
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
GSTATUS 
WPSTerminate () 
{
	GSTATUS err = GSTAT_OK;
	err = WPSFileTerminate();
return(err);
}
