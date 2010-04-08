/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <actsession.h>
#include <erwsf.h>
#include <wsmvar.h>

/*
**
**  Name: wpsapp.c - External Application Execution and Format
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
**      08-Aug-2000 (gupsh01)
**          Modified the WSM_PSCAN_VAR to WSM_SCAN_VAR. ( bug # 102319 )
**          also added STR_CMDLINE_QUOTE to the params string for each
**          value, as this was creating problems with characters in value
**          which need to be delimited, before being passed as parameters.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name: WPSAppAdditionalParams() - 
**
** Description:
**      Grab any user parameters from the HTML form
**
** Inputs:
**	ACT_PSESSION act_session
**
** Outputs:
**	char*	*params
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
**      07-Oct-98 (fanra01)
**          Remove spaces around the equal sign of parameters.
**          Replace spaces in values with the plus '+' character.
**      08-Aug-2000 (gupsh01)
**          Modified the WSM_PSCAN_VAR to WSM_SCAN_VAR. ( bug # 102319 )
**          also added STR_CMDLINE_QUOTE to the params string for each
**          value, as this was creating problems with characters in value
**          which need to be delimited, before being passed as parameters.
*/
GSTATUS
WPSAppAdditionalParams (
	ACT_PSESSION act_session,
	char*	*params)
{
	GSTATUS err = GSTAT_OK;
    char    *variable = NULL;
    char    *value = NULL;
    int     totalLen = 0;
	WSM_SCAN_VAR scanner;

    /* First count how big the final parameter string will be */
	err = WSMOpenVariable (&scanner, act_session, WSM_ACTIVE);
	while (err == GSTAT_OK) 
	{
		err = WSMScanVariable (&scanner, &variable, &value, NULL);
		if (err != GSTAT_OK || variable == NULL)
			break;
		if (value != NULL && value[0] != EOS)
            totalLen += STlength (variable) + STlength (value) +
			(STlength ( STR_CMDLINE_QUOTE ) * 2) +
                        (STlength (STR_SPACE) * 3) +
                        STlength (STR_EQUALS_SIGN);
	}
	WSMCloseVariable (&scanner);

    /* now assemble the parameters in the appropriate format,
    ** var1 = 'value1'{ var2 = 'value2'}
    */
    if (err == GSTAT_OK && totalLen > 0)
    {
        *params = EOS;
        err = G_ME_REQ_MEM(act_session->mem_tag, *params, char, totalLen + 1);
        if (err == GSTAT_OK)
        {
            err = WSMOpenVariable (&scanner, act_session, WSM_ACTIVE);
            while (err == GSTAT_OK)
            {
                err = WSMScanVariable (&scanner, &variable, &value, NULL);
                if (err != GSTAT_OK || variable == NULL)
                    break;
                if (value != NULL && value[0] != EOS)
                {
                    char*   src;
                    char*   dst;
                    char*   mem;
                    i4      i;
                    for (i=0, src=value; *src != EOS; CMnext(src))
                    {
                        if (*src == CHAR_PLUS) i+=1;
                    }
                    i+=STlength(value);
                    err = G_ME_REQ_MEM(act_session->mem_tag, mem, char, i);
                    if (err == GSTAT_OK)
                    {
                        for (i=0, src=value, dst=mem; *src != EOS; )
                        {
                            switch (*src)
                            {
                                case CHAR_PLUS:
                                    CMcpychar (STR_PLUS, dst);
                                    CMnext (dst);
                                case SPACE:
                                    CMcpychar (STR_PLUS, dst);
                                    CMnext (src);
                                    CMnext (dst);
                                    break;
                                default:
                                    CMcpyinc (src, dst);
                                    break;
                            }
                        }
                        *dst = EOS;
                     STpolycat (7, *params, variable, STR_EQUALS_SIGN,
                            STR_CMDLINE_QUOTE, mem, STR_CMDLINE_QUOTE, 
                            STR_SPACE, *params);
                        MEfree (mem);
                    }
                }
            }
            WSMCloseVariable (&scanner);
        }
        /* dump the trailing comma and space */
        STtrmwhite (*params);
    }
    return(err);
}

/*
** Name: WPSAppParentDirReference() - 
**
** Description:
**      Checks to see if application makes reference to a parent directory
**      (../ or ..\). If required, other platforms that have a diffrent ways
**      of representing a parent directory should be added here.
**
** Inputs:
**	char*	: application name
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
WPSAppParentDirReference (
	char *app,
	bool *ret)
{
    char    *index;

    if (index = STstrindex (app, STR_PERIOD, 0, TRUE))
        CMnext (index);
        if (index && (index = STstrindex (index, STR_PERIOD, 0, TRUE)))
            CMnext (index);
        if (index && (*index == CHAR_PATH_SEPARATOR))
            *ret = TRUE;
return (GSTAT_OK);
}

/*
** Name: WPSAppBegin() - 
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		 : application name
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
**      08-Oct-98 (fanra01)
**          Initialise the user field for inclusion in message.
*/
GSTATUS
WPSAppBegin (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer,
	char* name)
{
	GSTATUS err = GSTAT_OK;
    char*       pszMsg = NULL;
    char*       user = NULL;
	i4 cmtLen;	
	char cmt[256];

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
		CL_ERR_DESC clError;

        if (act_session && act_session->user_session)
        {
            user = act_session->user_session->user;
        }
        else
        {
            user = STR_UNKNOWN_USER;
        }
		ERslookup (
				I_WS0041_AGEN_ICE_CMT, 
				NULL, 
				0, 
				NULL, 
				cmt, 
				sizeof(cmt),
				-1, 
				&cmtLen, 
				&clError, 
				0, 
				NULL);
		cmt[cmtLen] = EOS;

		err = G_ME_REQ_MEM(act_session->mem_tag, pszMsg, char, cmtLen + 
            STlength (user) + STlength (name) + 1);
		if (err == GSTAT_OK)
		{
			STprintf (pszMsg, cmt, user, name);
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

    if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_HEAD_TITLE, 
				STlength(TAG_HEAD_TITLE));

	if (err == GSTAT_OK)
	{
		CL_ERR_DESC clError;
		ERslookup (
				I_WS0038_DEFAULT_HTML_TITLE, 
				NULL, 
				0, 
				NULL, 
				cmt, 
				sizeof(cmt),
				-1, 
				&cmtLen, 
				&clError, 
				0, 
				NULL);
		cmt[cmtLen] = EOS;

		err = WPSBlockAppend(
				buffer, 
				cmt, 
				STlength(cmt));
	}
	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_END_HEAD_TITLE, 
				STlength(TAG_END_HEAD_TITLE));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_BODY, 
				STlength(TAG_BODY));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_PREFORM, 
				STlength(TAG_PREFORM));

return (err);
}

/*
** Name: WPSAppEnd() - 
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
WPSAppEnd (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer)
{
	GSTATUS err = GSTAT_OK;

	err = WPSBlockAppend(
			buffer, 
			STR_NEWLINE, 
			STlength(STR_NEWLINE));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
			buffer, 
			TAG_END_PREFORM, 
			STlength(TAG_END_PREFORM));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_END_BODY, 
				STlength(TAG_END_BODY));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(
				buffer, 
				TAG_END_HTML, 
				STlength(TAG_END_HTML));
return (err);
}
