/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wpsreport.h>
#include <wcs.h>
#include <erwsf.h>
#include <wsmvar.h>
#include <wpsfile.h>

#include <dds.h>

/*
**
**  Name: wpsreport.c - Ingres Report Support
**
**  Description:
**	That file provides functions to execute RBF of Ingres. 
**	It supports all the features of OpenIngres ICE 20
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      06-Oct-98 (fanra01)
**          Fix exception in WPSGetReportParameters.
**          Fix exception in WPSGetOutputDirLoc.
**      01-Dec-1998 (fanra01)
**          Add use of alias for db user id when accessing reports.
**      18-Jan-1999 (fanra01)
**          Add dds.h for prototypes.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Add additional scheme parameter to WPSRequest.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  20-Aug-2003 (fanra01)
**      Bug 110747
**      Add null parameter checks.
**/

/*
** Name: WPSGetReportParameters() - Format report parameter
**
** Description:
**	browse page variables and put them into the formatted string
**	allocate the nessecary memory
**
** Inputs:
**	ACT_PSESSION : active session
**
** Outputs:
**	char**        : formatted parameter string
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
**      06-Oct-1998 (fanra01)
**          Allocate space for WSM_SCAN_VAR on the stack instead of using
**          a pointer.
*/

GSTATUS
WPSGetReportParameters (
	ACT_PSESSION act_session,
	char*       *pszParams)
{
	GSTATUS		err = GSTAT_OK;
    char*       variable = NULL;
    char*       value = NULL;
    int         totalLen = 0;
	WSM_SCAN_VAR scanner;

	err = WSMOpenVariable (&scanner, act_session, WSM_ACTIVE);
	while (err == GSTAT_OK) 
	{
		err = WSMScanVariable (&scanner, &variable, &value, NULL);
		if (err != GSTAT_OK || variable == NULL)
			break;
		if (value != NULL && value[0] != EOS)
			totalLen += STlength (variable) + STlength (value) + REP_PARAM_DELIMITER_LEN + 1;
	}
	WSMCloseVariable (&scanner);

    /* now assemble the parameters in the appropriate format,
    ** var1='value1'[, var2='val2']
    */
    if ((err == GSTAT_OK) && (pszParams != NULL) && totalLen > 0)
    {
        err = G_ME_REQ_MEM(act_session->mem_tag, *pszParams, char, totalLen + 1);
		if (err == GSTAT_OK)
        {
            /*
            ** Move initialization of string to after allocation.
            ** Add indirection as pszParams is a pointer to a pointer.
            */            
            *(*pszParams) = EOS;
			err = WSMOpenVariable (&scanner, act_session, WSM_ACTIVE);
			while (err == GSTAT_OK) 
			{
				err = WSMScanVariable (&scanner, &variable, &value, NULL);
				if (err != GSTAT_OK || variable == NULL)
					break;
				if (value != NULL && value[0] != EOS)
					STpolycat (7, *pszParams, variable, STR_EQUALS_SIGN,
                           STR_CMDLINE_QUOTE, value, STR_CMDLINE_QUOTE,
                           STR_PARAM_SEPARATOR, *pszParams);
			}
			WSMCloseVariable (&scanner);
        }
        /* dump the trailing comma and space */
        (*pszParams) [totalLen - 2] = EOS;
    }
return(err);
}

/*
** Name: WPSGetOutputDirLoc() - Determine the output and error files to execute the report
**
** Description:
**	Define the correct location by using ICE 20 way
**	Request the file 
**
** Inputs:
**	ACT_PSESSION : active session
**
** Outputs:
**	WPS_PFILE *	: output file
**	WPS_PFILE *	: error file
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
**      06-Oct-1998 (fanra01)
**          Initialise the correct level of the output location.
**          Set the directory when not sepcified.
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSRequest.
*/
GSTATUS
WPSGetOutputDirLoc (
	ACT_PSESSION act_session,
	WPS_PFILE *ploOut,
	WPS_PFILE *ploErr)
{
	GSTATUS				err = GSTAT_OK;
    char*               pszDir = NULL;
    bool				status;
    char dev[MAX_LOC+1];
    char path[MAX_LOC+1];
    char prefix[MAX_LOC+1];
    char suffix[MAX_LOC+1];
    char vers[MAX_LOC+1];

    /* Choose which of the output subdirectories to use. Because the
    ** way of specifying the output directory changed between v1.2 and
    ** 2.0, we have to support both. The V2 method takes priority
    */
	*ploOut = NULL;

    err = WSMGetVariable (
        act_session,
        HVAR_OUTPUT_DIR_LABEL,
        STlength(HVAR_OUTPUT_DIR_LABEL),
        &pszDir,
        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    if (err == GSTAT_OK && pszDir != NULL)
    {
        err= WPSRequest(
            (act_session->user_session != NULL) ?
                act_session->user_session->name : NULL,
            act_session->scheme,
            act_session->host,
            act_session->port,
            pszDir,
            SYSTEM_ID,
            NULL,
            WSF_LOG_SUFFIX,
            ploOut,
            &status);

        if (err == GSTAT_OK && ploErr != NULL)
            err= WPSRequest(
                (act_session->user_session != NULL) ?
                    act_session->user_session->name : NULL,
                act_session->scheme,
                act_session->host,
                act_session->port,
                pszDir,
                SYSTEM_ID,
                NULL,
                WSF_ERROR_SUFFIX,
                ploErr,
                &status);
    }

    if (err == GSTAT_OK && pszDir == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_REPORT_LOCATION,
            STlength(HVAR_REPORT_LOCATION),
            &pszDir,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (err == GSTAT_OK && pszDir != NULL)
        {
            err= WPSRequest(
                (act_session->user_session != NULL) ?
                    act_session->user_session->name : NULL,
                    act_session->scheme,
                    act_session->host,
                    act_session->port,
                    pszDir,
                    SYSTEM_ID,
                    NULL,
                    WSF_LOG_SUFFIX,
                    ploOut,
                    &status);

            if (err == GSTAT_OK && ploErr != NULL)
                err= WPSRequest(
                    (act_session->user_session != NULL) ?
                        act_session->user_session->name : NULL,
                        act_session->scheme,
                        act_session->host,
                        act_session->port,
                        pszDir,
                        SYSTEM_ID,
                        NULL,
                        WSF_ERROR_SUFFIX,
                        ploErr,
                        &status);
        }
    }

    if (err == GSTAT_OK && pszDir == NULL)
    {
        err = WSMGetVariable (
            act_session,
            HVAR_OUTPUT_DIR_NUM,
            STlength(HVAR_OUTPUT_DIR_NUM),
            &pszDir,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
        if (err == GSTAT_OK && pszDir != NULL)
        {
            char *tmp;
            err = G_ME_REQ_MEM(
                act_session->mem_tag,
                tmp,
                char,
                STlength(HVAR_OUTPUT_DIR_NUM) + STlength(pszDir) + 1);
            if (err == GSTAT_OK)
            {
                STprintf (tmp, CONF_OUTPUT_DIR_LOC, PMhost(), pszDir);
                err= WPSRequest(
                    (act_session->user_session != NULL) ?
                        act_session->user_session->name : NULL,
                    act_session->scheme,
                    act_session->host,
                    act_session->port,
                    tmp,
                    SYSTEM_ID,
                    NULL,
                    WSF_LOG_SUFFIX,
                    ploOut,
                    &status);

                if (err == GSTAT_OK && ploErr != NULL)
                    err= WPSRequest(
                        (act_session->user_session != NULL) ?
                            act_session->user_session->name : NULL,
                        act_session->scheme,
                        act_session->host,
                        act_session->port,
                        pszDir,
                        SYSTEM_ID,
                        NULL,
                        WSF_ERROR_SUFFIX,
                        ploErr,
                        &status);
                MEfree(tmp);
            }
        }
    }

    if (err == GSTAT_OK && pszDir == NULL)
    {
        err= WPSRequest(
            (act_session->user_session != NULL) ?
                act_session->user_session->name : NULL,
            act_session->scheme,
            act_session->host,
            act_session->port,
            NULL, 
            SYSTEM_ID, 
            NULL, 
            WSF_LOG_SUFFIX, 
            ploOut, 
            &status);

        if (err == GSTAT_OK)
        {
            if (LOdetail (&((*ploOut)->file->loc), dev, path, prefix, suffix, vers) == OK)
            {
                pszDir = path;
            }            
        }
        if (err == GSTAT_OK && ploErr != NULL)
            err= WPSRequest(
                (act_session->user_session != NULL) ?
                    act_session->user_session->name : NULL,
                act_session->scheme,
                act_session->host,
                act_session->port,
                NULL,
                SYSTEM_ID,
                NULL,
                WSF_ERROR_SUFFIX,
                ploErr,
                &status);
    }

    if (err == GSTAT_OK)
        if (pszDir == NULL)
            err = DDFStatusAlloc(E_WS0035_INST_DIR_NOEXIST);
    return (err);
}

/*
** Name: WPSGetReportCommand() - Create the report command
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		: dbname
**	char*		: name
**	char*		: location
**	char*		: params
**	LOCATION*	: ploOut
**
** Outputs:
**	char**		: command
**	u_nat*		: number of parameter
**	char**		: table of parameters
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
**      06-Oct-98 (fanra01)
**          Change condition for adding user parameters to reports.
**      01-Dec-1998 (fanra01)
**          Add use of alias for db user id when accessing reports.
*/
GSTATUS
WPSGetReportCommand (
    char*    *command,
    u_i4    *ixParam,
    char*    *apszParam,
    ACT_PSESSION act_session,
    char*    dbname,
    char*    name,
    char*    location,
    char*    params,
    LOCATION*    ploOut)
{
    GSTATUS        err = GSTAT_OK;
    char*       pszCmdFmt = NULL;
    int         lenFmt;

    *ixParam = 0;
    lenFmt = STlength (STR_RW_DATABASE_PARAM) +
             STlength (STR_RW_USER_PARAM) +
             STlength (STR_RW_FLAGS_PARAM) +
             STlength (STR_RW_FILE_PARAM) +
             STlength (STR_RW_SRCFILE_PARAM) +
             STlength (STR_RW_REPNAME_PARAM) +
             STlength (STR_RW_MISC_PARAM) + 1;

    err = G_ME_REQ_MEM(act_session->mem_tag, pszCmdFmt, char, lenFmt);
    if (err == GSTAT_OK)
    {
        apszParam[(*ixParam)++] = dbname;
        STcopy (STR_RW_DATABASE_PARAM, pszCmdFmt);

        /* we only pass the user ID to the report writer if the database
        ** is _not_ accessed through a gateway
        **/
        if (STstrindex (dbname, STR_GATEWAY_SEPARATOR, 0, FALSE) == NULL)
        {
            if ( act_session != NULL && act_session->user_session != NULL)
            {
                apszParam[(*ixParam)++] = act_session->user_session->user;
            }
            else
            {
                char* user = NULL;
                char* dbuser = NULL;
                char* dbpassword = NULL;

		err = WSMGetVariable (act_session, HVAR_USERID,
                    STlength(HVAR_USERID), &user,
                    WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
                if (err == GSTAT_OK)
                {
                    err = DDSGetDBUserPassword (user, &dbuser, &dbpassword);
                    if ((err == GSTAT_OK) && (user != NULL))
                        apszParam[(*ixParam)++] = dbuser;
                }
            }
            if (err == GSTAT_OK)
                STcat (pszCmdFmt, STR_RW_USER_PARAM);
        }
        /* pass the required flags */
        apszParam[(*ixParam)++] = STR_ING_RW_FLAGS;
        STcat (pszCmdFmt, STR_RW_FLAGS_PARAM);

        /* pass the output file name */
        LOtos (ploOut, &(apszParam[(*ixParam)++]));
        STcat (pszCmdFmt, STR_RW_FILE_PARAM);

        /* We pass either the location of a file containing a RW script
        ** or the name of a report stored in the database. The script
        ** location takes priority */
        if (location != NULL)
        {
            apszParam[(*ixParam)++] = location;
            STcat (pszCmdFmt, STR_RW_SRCFILE_PARAM);
        }
        else
        {
            apszParam[(*ixParam)++] = name;
            STcat (pszCmdFmt, STR_RW_REPNAME_PARAM);
        }
        /* pass optional parameters derived from HTML variables */
        if (params != NULL && params[0] != EOS)
        {
            apszParam[(*ixParam)++] = params;
            STcat (pszCmdFmt, STR_RW_MISC_PARAM);
        }
        /* remove the trailing ", " from the command fmt string */
        pszCmdFmt[STlength (pszCmdFmt) - 2] = EOS;
    }
    if (err == GSTAT_OK)
        *command = pszCmdFmt;
return (err);
}

/*
** Name: WPSReportError() - Format into the HTML buffer the report error 
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		: command
**	char**		: table of parameters
**	WPS_FILE*	: error file
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
**      30-Nov-1998 (fanra01)
**          Use initialised file descriptor.
*/
GSTATUS
WPSReportError (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char*		command,
	char*		*apszParam,
	WPS_FILE	*pploErr)
{
	GSTATUS			err = GSTAT_OK;
	GSTATUS			status = GSTAT_OK;
	char*			line = NULL;
    char*           value;
    FILE*           fp;
	u_i4			counter = 0;

	err = G_ME_REQ_MEM(act_session->mem_tag, line, char, SI_MAX_TXT_REC+1);
	if (err == GSTAT_OK)
	{
		err = WCSOpen (pploErr->file, "r", SI_BIN, 0);
		if (err == GSTAT_OK)
		{
                        fp = pploErr->file->fd;
			/*
			** Get Report-Writer-generated error message and format it 
			** so we can use it in the log file.
			*/
			while (	err == GSTAT_OK && 
					(SIgetrec (line, SI_MAX_TXT_REC, fp) == OK) &&
					line[0] != EOS)
			{
				line [STlength (line)] = EOS;
				value = line;

				while (*value == ' ')
						CMnext (value);

				if (value != NULL && value[0] != EOS)
				{
					if (counter++ != 0)
						err = WPSBlockAppend(
								buffer, 
								TAG_PARAGRAPH_BREAK, 
								STlength(TAG_PARAGRAPH_BREAK));

					if (err == GSTAT_OK)
						err = WPSBlockAppend(
								buffer, 
								value, 
								STlength(value));
				}
			}
			CLEAR_STATUS( WCSClose(pploErr->file) );
			MEfree(line);
		}
    }

	if (err == GSTAT_OK)
	{
		/* Set up an error message to send to the client and/or the ICE log file */
		status = DDFStatusAlloc(E_WS0036_BAD_RW_EXECUTION);
        /*
        ** Test all parameter pointers for null
        */
		err = DDFStatusInfo(status, command, 
            (apszParam[0]) ? apszParam[0] : "\0", 
            (apszParam[1]) ? apszParam[1] : "\0", 
            (apszParam[2]) ? apszParam[2] : "\0", 
            (apszParam[3]) ? apszParam[3] : "\0", 
            (apszParam[4]) ? apszParam[4] : "\0", 
            (apszParam[5]) ? apszParam[5] : "\0");
		if (err == GSTAT_OK)
		{
			CL_ERR_DESC clError;
			i4 nMsgLen;	
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

			err = WPSBlockAppend(
					buffer, 
					szMsg, 
					nMsgLen);
		}
		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					status->info, 
					STlength(status->info));

	    if (err == GSTAT_OK)
			DDFStatusFree(TRC_EXECUTION, &status);
	}
return(err);
}

/*
** Name: WPSReportBegin() - Format into the HTML the report result
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		: report name
**	char*		: locaiton name
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
**      06-Oct-1998 (fanra01)
**          Get user from act_session.
*/
GSTATUS
WPSReportBegin (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name,
	char* location)
{
    GSTATUS     err = GSTAT_OK;
    char*       value = NULL;
    char*       pszMsg = NULL;
    i4     cb = 0;
    CL_ERR_DESC clError;
    i4          nMsgLen;	
    char        szMsg[ERROR_BLOCK];
    char*       user = NULL;

    /* Write the document type to the output file */
    err = WPSBlockAppend(
        buffer,
        TAG_HTML_DTD,
        STlength(TAG_HTML_DTD));

    if (err == GSTAT_OK)
        err = WPSBlockAppend(
            buffer,
            TAG_COMMENT_BEGIN,
            STlength(TAG_COMMENT_BEGIN));

    if (err == GSTAT_OK)
        err = WPSBlockAppend(
            buffer,
            STR_NEWLINE,
            STlength(STR_NEWLINE));

    if (err == GSTAT_OK)
    {
        if ((value = name) == NULL)
            value = location;
        if (value == NULL)
            value = STR_UNKNOWN_REPORT;
        if (act_session && act_session->user_session)
        {
            user = act_session->user_session->user;
        }
        else
        {
            user = STR_UNKNOWN_USER;
        }        
        ERslookup (
            I_WS0037_RGEN_ICE_CMT,
            NULL,
            0,
            NULL,
            szMsg,
            sizeof(szMsg),
            -1,
            &nMsgLen,
            &clError,
            0,          
            NULL);

        szMsg[nMsgLen] = EOS;

        err = G_ME_REQ_MEM(act_session->mem_tag, pszMsg, char, nMsgLen + 
            STlength (user) + STlength (value) + 1);
        if (err == GSTAT_OK)
        {
            STprintf (pszMsg, szMsg, user, value);
            err = WPSBlockAppend(
                buffer,
                pszMsg,
                STlength(pszMsg));

            MEfree (pszMsg);
        }
    }
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_COMMENT_END, 
				STlength(TAG_COMMENT_END));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				STR_NEWLINE, 
				STlength(STR_NEWLINE));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_HTML, 
				STlength(TAG_HTML));

    /* Write out the report header if one exists */
	err = WSMGetVariable (act_session, HVAR_REPORT_HEADER, STlength(HVAR_REPORT_HEADER), &value, WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
    if (err == GSTAT_OK && value != NULL)
    {
		err = WPSBlockAppend(
					buffer, 
					TAG_HEAD_TITLE, 
					STlength(TAG_HEAD_TITLE));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					value, 
					STlength(value));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					TAG_END_HEAD_TITLE, 
					STlength(TAG_END_HEAD_TITLE));
    }
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
					buffer, 
					TAG_BODY, 
					STlength(TAG_BODY));

    /* Write out the page header if one exists */
	err = WSMGetVariable (act_session, HVAR_PAGE_HEADER, STlength(HVAR_PAGE_HEADER), &value, WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
    if (err == GSTAT_OK && value != NULL)
    {
		err = WPSBlockAppend(
					buffer, 
					TAG_HEADER1, 
					STlength(TAG_HEADER1));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					value, 
					STlength(value));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					TAG_END_HEADER, 
					STlength(TAG_END_HEADER));
    }
    /* write the report as preformatted text */
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
					buffer, 
					TAG_PREFORM, 
					STlength(TAG_PREFORM));

return (err);
}

/*
** Name: WPSReportEnd() - Format into the HTML the report result
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
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
WPSReportEnd (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer)
{
	GSTATUS err = GSTAT_OK;
	err = WPSBlockAppend(
				buffer, TAG_END_PREFORM, STlength(TAG_END_PREFORM));
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, TAG_END_BODY, STlength(TAG_END_BODY));
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, TAG_END_HTML, STlength(TAG_END_HTML));
return (err);
}

/*
** Name: HasTags() - check if a buffer has HTML code
**
** Description:
**
** Inputs:
**	char*	: buffer
**
** Outputs:
**	bool*	: result
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
HasTags (
	char* pszBuf, 
	bool *ret)
{
    char*       pszTag;
    int         i;
    char*       pszBeg = NULL;
    char*       pszEnd = NULL;
    char*       aszHTMLTags[] =
    {
        "<html>",
        "<head>",
        "<title>",
        "<body>"
    };

	*ret = FALSE;
    for (i = 0; i < (sizeof(aszHTMLTags)/sizeof(*aszHTMLTags)); i++)
    {
        /* look for a tag */
        if (pszTag = STstrindex (pszBuf, aszHTMLTags[i], 0, TRUE))
        {
            /* Find if we are in a comment. Look for the last open comment
            ** before the tag
            */
            pszBeg = STrstrindex (pszBuf, TAG_COMMENT_BEGIN, pszTag - pszBuf, 
                                  TRUE);
            {
                /* Now look for the first close comment after the tag */
                pszEnd = STstrindex (pszBeg, TAG_COMMENT_END, 0, TRUE);
                if ((pszBeg < pszTag) && (pszTag < pszEnd))
                {
                    /* the tag we found is between the start and
                    ** end of a comment, so we ignore it
                    */
                    pszTag = NULL;
                }
            }
        }
        if (pszTag)
        {
            *ret = TRUE;
            break;
        }
    }
return(GSTAT_OK);
}

/*
** Name: WPSReportSuccess() - Success report execution
**
** Description:
**	read stdout file of the report execution and format into the 
**	active session buffer.
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		 : report name 
**	char*		 : locaiton name 
**	WPS_FILE*    : output report file
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
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings.
*/
GSTATUS
WPSReportSuccess (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name,
	char* location,
	WPS_FILE*   ploOut)
{
    GSTATUS				err = GSTAT_OK;
    char*               pszOutBuf;
    bool				hasTag = FALSE;
    i4				length = 0;

    /* allocate a buffer into which some or all of the file will be
    ** read
    */
    err = G_ME_REQ_MEM(act_session->mem_tag, pszOutBuf, char, SI_MAX_TXT_REC+1);
    if (err == GSTAT_OK)
    {
	/* open the report file */
	err = WCSOpen (ploOut->file, "r", SI_BIN, 0);
	if (err == GSTAT_OK)
	{
	    err = WCSRead (ploOut->file, SI_MAX_TXT_REC, &length, pszOutBuf);
	    if (err == GSTAT_OK && length > 0)
	    {
		pszOutBuf[length] = EOS;
		err = HasTags(pszOutBuf, &hasTag);
			
		if (err == GSTAT_OK && hasTag == FALSE)
		    err = WPSReportBegin(act_session, buffer, name, location);

		if (err == GSTAT_OK)
		    err = WPSBlockAppend(
					 buffer, 
					 pszOutBuf, 
					 length);

		err = WCSRead (ploOut->file, SI_MAX_TXT_REC, &length, pszOutBuf);
		while (	err == GSTAT_OK && 
			length > 0)
		{
		    err = WPSBlockAppend(
					 buffer, 
					 pszOutBuf, 
					 length);

		    if (err == GSTAT_OK)
			err = WCSRead (ploOut->file, SI_MAX_TXT_REC, &length, pszOutBuf);
		}

		if (err == GSTAT_OK && hasTag == FALSE)
		    err = WPSReportEnd(act_session, buffer);
	    }
	    CLEAR_STATUS( WCSClose(ploOut->file) );
	}
	MEfree (pszOutBuf);
    }
    return(err);
}
