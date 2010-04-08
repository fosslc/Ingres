/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <htmlmeth.h>
#include <usrsession.h>
#include <dbaccess.h>
#include <wsspars.h>
#include <wss.h>
#include <servermeth.h>
#include <htmlpars.h>
#include <erwsf.h>
#include <dds.h>
#include <wcs.h>
#include <wsmvar.h>
#include <wpsfile.h>

#include <asct.h>

/*
**
**  Name: htmlmeth.c - html macro execution
**
**  Description:
**	that file provides parser associated functions to execute ICE macros
**
**  History:
**	5-Feb-98 (marol01)
**	    created
**  12-Aug-98 (fanra01)
**          Renamed clashing defines.
**  08-Sep-1998 (fanra01)
**          Corrected case of urssession to match piccolo.
**          Fixed compiler errors on unix.
**  10-Sep-98 (marol01)
**      added HVAR_DATABASE as a default database name.
**  15-Sep-98 (marol01)
**      fix sql quote issue.
**  08-Oct-98 (fanra01)
**      When preparing to return an error to the client ensure that the
**      message is surrounded by html and body tags.
**  20-Oct-98 (fanra01)
**      Change result type to signed for comparisons in WPSHtmlIfLogic.
**      Remove trailing spaces from text columns.
**  28-Oct-98 (fanra01)
**      Add WPSHtmlLibName to retrieve the library name and skip the '.'
**      delimiter.
**  18-Nov-1998 (fanra01)
**      Remove two memory leaks.
**  10-Mar-1999 (fanra01)
**      Add check for external facet and read message into buffer if its not
**      available as a url.
**  11-Mar-1999 (fanra01)
**      Add check for null data in WPSHtmlSingleQuery before attempting trim.
**  17-Mar-1999 (fanra01)
**      Remove additional check for formating NULL data as function
**      should deal with NULL values.
**  06-Apr-1999 (fanra01)
**      For 2.0 queries with blobs the HTML output would use the OS path to
**      reference the object.  If the url exists use it in preference to the
**      OS path.
**  15-Apr-1999 (fanra01)
**      When retrieving blobs test the filename and extension returned from
**      DBText.  Attempt to use the value from the EXT option if the extension
**      is unknown.
**  18-Mar-1999 (peeje01)
**      Add tracing for user functions
**  14-May-1999 (fanra01)
**      Update WPSHtmlSetInfo to ignore non-existant and empty variables.
**  18-Jun-1999 (fanra01)
**      Backout previous change and revert to original behaviour.
**  04-Jan-2000 (fanra01)
**      Bug 99925
**      Add test in WPSHtmlSqlParam for query pointer validity before using.
**  28-Apr-2000 (fanra01)
**      Bug 100312
**      Add SSL support.  Add additional scheme parameter to WPSrequest.
**  05-Jun-2000 (fanra01)
**      Bug 101462
**      If the parser comes across a character it thinks terminates a statement
**      the function WPSHtmlNewSqlQuery scans the statement for non-terminated
**      quotes.  If an odd number of quotes or parentheses is encountered
**      the function does not create a new statement but continues the current
**      one.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  10-Oct-2000 (fanra01)
**      Bug 102882
**      Set nb_of_columns accroding to function parameter number as count
**      could be remnant of previous query stepping beyond the bounds of
**      the tag array and causing an exception.
**  11-Oct-2000 (fanra01)
**      Sir 103096
**      Add functions to provide basic XML output support based on the HTML
**      option for formatting.
**      WPSXmlTypeRaw       set query type to XML
**      WPSMarkupText       text output for XML format
**      WPSXmlMarkupVar     variable output for XML format
**  02-Dec-2000 (fanra01)
**      Bug 102857
**      Add function WPSAppendCharToValue to take the current character
**      append it to the value string of a variable.
**      Add length check to variable test in WPSHtmlSetInfo.
**      23-Jan-2001 (hanje04, somsa01)
**          Added Null check on pout->version25.value to WPSHtmlDecl()
**          when calling WSMAddVariable because STlength(NULL) will SEGV
**          if STlength is defined to be strlen(). For
**          pout->version25.value == NULL, we now pass 0. Also, in
**          WPSHtmlIfLogic(), this is also holds when STcompare() is
**          defined to be strcmp().
**      20-Mar-2001 (fanra01)
**          Correct function spelling of WCSDispatchName.
**      12-Jun-2001 (fanra01)
**          Sir
**          Add WPSXmlTypeXml   set output format type to XML.
**      30-Oct-2001 (fanra01)
**          Bug 106215
**          Changed profile messages with authorization and permission
**          messages.
**      30-Oct-2001 (fanra01)
**          Correct error number definition from previous change to reduce
**          identifier truncation.
**      20-Mar-2002 (fanra01)
**          Bug 107368
**          Memory leak, leaving dirty buffers around after processing each
**          query.
**      20-Aug-2003 (fanra01)
**          Bug 110747
**          Additional output information on encountering a parser error. 
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

GSTATUS
WPSPerformVariable (
	ACT_PSESSION act_session,
	WPS_PBUFFER	 buffer);

/*
** Name: WPSConvertTag() - Convert tags to String
**
** Description:
**
** Inputs:
**	char*			: tags
**	u_i4			: length of the source string. expected null string if -1
**
** Outputs:
**	char*			: string
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
WPSConvertTag  (
	char*	source,
	u_i4	src_len, 
	char*	*destination,
	u_i4	*dest_len,
	bool	keep_new_line)
{
	GSTATUS		err = GSTAT_OK;
	u_i4		length = 1;
	char*		tmp = source;

	while (tmp != NULL && tmp[0] != EOS)
	{
		if (tmp[0] == '<' || 
			tmp[0] == '>')
			length += 4;
		else if (tmp[0] == '\n' && keep_new_line == TRUE)
				length += 4;
		else
			length++;
		tmp++;
	}

	err = GAlloc((PTR*)destination, length, FALSE);
	if (err == GSTAT_OK)
	{
		(*destination)[0] = EOS;
		tmp = source;
		if (tmp != NULL)
		{
			length = 0;
			while (*tmp != EOS)
			{
				switch (*tmp)
				{
				case '<' :
					(*destination)[length++] = '&';
					(*destination)[length++] = 'l';
					(*destination)[length++] = 't';
					(*destination)[length++] = ';';
					break;
				case '>' :
					(*destination)[length++] = '&';
					(*destination)[length++] = 'g';
					(*destination)[length++] = 't';
					(*destination)[length++] = ';';
					break;
				case '\n' :
					if (keep_new_line == TRUE)
					{
						(*destination)[length++] = '<';
						(*destination)[length++] = 'B';
						(*destination)[length++] = 'R';
						(*destination)[length++] = '>';
					}
					else
						(*destination)[length++] = tmp[0];
					break;
				default:
					(*destination)[length++] = tmp[0];
				}
				(*destination)[length] = EOS;
				tmp++;
			}
		}
	}
	if (dest_len != NULL)
		*dest_len = length;
return(err);
}

/*
** Name: WPSPlainText() - Set the out structure with an error and put the error
**						 into a html comment
**
** Description:
**
** Inputs:
**	WPS_HTML_PPARSER_OUT	: out parser structure
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
**      18-Nov-1998 (fanra01)
**          Free allocated memory returned from WPSConvertTag.
*/
GSTATUS
WPSPlainText (
    WPS_PBUFFER destination,
    WPS_PBUFFER source)
{
    GSTATUS err = GSTAT_OK;
    char*   str = NULL;
    char*   converted = NULL;
    u_i4   read = 0;
    i4 max = 0;

    do
    {
        err = WPSBufferGet(source, &read, &max, &str, 0);
        if (err != GSTAT_OK || read <= 0)
            break;

        err = WPSConvertTag(str, read, &converted, &read, TRUE);
        if (err == GSTAT_OK)
        {
            err = WPSBlockAppend(destination, converted, read);
            MEfree (converted);
        }
    }
    while (err == GSTAT_OK);
    return(err);
}

/*
** Name: WPSSetError() - Set the out structure with an error and put the error
**						 into a html comment
**
** Description:
**
** Inputs:
**	WPS_HTML_PPARSER_OUT	: out parser structure
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
**          When preparing to return an error to the client ensure that the
**          message is surrounded by html and body tags.
*/
GSTATUS
WPSSetError (
    WPS_HTML_PPARSER_OUT pout,
    u_i4 type)
{
    GSTATUS err = GSTAT_OK;
    char    number[NUM_MAX_INT_STR];
    char    *msgError;
    char    szMsg[ERROR_BLOCK];

    err = WSMGetVariable (
        pout->act_session,
        HVAR_ERROR_MSG,
        STlength(HVAR_ERROR_MSG),
        &msgError,
        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

    if (msgError == NULL || msgError[0] == EOS)
    {
        CL_ERR_DESC clError;
        i4  nMsgLen = 0;
        if (type == ERROR_LVL_ERROR)
            pout->act_session->error_counter++;

        ERslookup (
            pout->error->number,
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
        msgError = szMsg;
    }

    if (err == GSTAT_OK)
    {
        bool addBody = FALSE;
        if (pout->result->position == 1)
        {
            err = WPSBlockAppend(
                pout->result,
                TAG_HTML,
                STlength(TAG_HTML));
            if (err == GSTAT_OK)
                err = WPSBlockAppend(
                    pout->result,
                    TAG_BODY,
                    STlength(TAG_BODY));
            addBody = TRUE;
        }
        err = WPSBlockAppend(
            pout->result,
            TAG_COMMENT_BEGIN,
            STlength(TAG_COMMENT_BEGIN));

        if (err == GSTAT_OK)
            switch (type)
            {
                case ERROR_LVL_ERROR:
                    err = WPSBlockAppend(
                        pout->result,
                        STR_ICE_ERROR_MSG_FILTER,
                        STlength(STR_ICE_ERROR_MSG_FILTER));
                    break;
                case ERROR_LVL_WARNING:
                    err = WPSBlockAppend(
                        pout->result,
                        STR_ICE_WARNING_MSG_FILTER,
                        STlength(STR_ICE_WARNING_MSG_FILTER));
                        break;
                default:;
            }

        if (err == GSTAT_OK)
        {
            err = WPSBlockAppend(
                pout->result,
                msgError,
                STlength(msgError));
            if (err == GSTAT_OK && pout->error->info != NULL)
            {
                err = WPSBlockAppend(
                    pout->result,
                    " ",
                    1);
                if (err == GSTAT_OK)
                    err = WPSBlockAppend(
                        pout->result,
                        pout->error->info,
                        STlength(pout->error->info));
            }
        }
        if (err == GSTAT_OK)
            err = WPSBlockAppend(
                pout->result,
                TAG_COMMENT_END,
                STlength(TAG_COMMENT_END));

        if (err == GSTAT_OK && addBody == TRUE)
        {
            err = WPSBlockAppend(
                pout->result,
                TAG_END_BODY,
                STlength(TAG_END_BODY));
            if (err == GSTAT_OK)
                err = WPSBlockAppend(
                    pout->result,
                    TAG_END_HTML,
                    STlength(TAG_END_HTML));
        }
    }

    if (err == GSTAT_OK)
    {
        CVna(pout->error->number, number);
        err = WSMAddVariable(
            pout->act_session,
            HVAR_STATUS_NUMBER,
            STlength(HVAR_STATUS_NUMBER),
            number,
            STlength(number),
            WSM_ACTIVE);
    }

    if (err == GSTAT_OK)
        err = WSMAddVariable(
            pout->act_session,
            HVAR_STATUS_TEXT,
            STlength(HVAR_STATUS_TEXT),
            msgError,
            STlength(msgError),
            WSM_ACTIVE);

    if (err == GSTAT_OK )
        err = WSMAddVariable(
            pout->act_session,
            HVAR_STATUS_INFO,
            STlength(HVAR_STATUS_INFO),
            (pout->error->info == NULL) ? "" : pout->error->info,
            (pout->error->info == NULL) ? 0 : STlength(pout->error->info),
            WSM_ACTIVE);

    DDFStatusFree(TRC_INFORMATION, &pout->error);
    pout->active_error = TRUE;
    return(err);
}

/*
** Name: WPSSetSuccess() - Initialize status variables
**
** Description:
**
** Inputs:
**	WPS_HTML_PPARSER_OUT	: out parser structure
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
WPSSetSuccess (
	WPS_HTML_PPARSER_OUT	pout) 
{
	GSTATUS	err = GSTAT_OK;
	char *msgSuccess;
	err = WSMGetVariable (
						pout->act_session, 
						HVAR_SUCCESS_MSG, 
						STlength(HVAR_SUCCESS_MSG), 
						&msgSuccess, 
						WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

	if (err == GSTAT_OK && msgSuccess != NULL)
	{
		err = WSMAddVariable(
					pout->act_session,
					HVAR_STATUS_NUMBER,
					STlength(HVAR_STATUS_NUMBER), 
					NULL_ERROR_STR,
					STlength(NULL_ERROR_STR),
					WSM_ACTIVE);

		if (err == GSTAT_OK)
			err = WSMAddVariable(
					pout->act_session,
					HVAR_STATUS_TEXT,
					STlength(HVAR_STATUS_TEXT), 
					msgSuccess,
					STlength(msgSuccess),
					WSM_ACTIVE);

		if (err == GSTAT_OK)
			err = WSMAddVariable(
					pout->act_session,
					HVAR_STATUS_INFO,
					STlength(HVAR_STATUS_INFO), 
					NULL,
					0,
					WSM_ACTIVE);
	}
	pout->active_error = FALSE;
return(err);
}

/*
** Name: WPSHtmlGetVar() - retrieve the varaible value
**
** Description:
**
** Inputs:
**	WPS_HTML_PPARSER_OUT	: out parser structure
**	char*					: variable name
**	u_i4					: variable name length,
**
** Outputs:
**	char**					: value
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
GSTATUS WPSHtmlGetVar (
	WPS_HTML_PPARSER_OUT	pout,
	char			*name,
	u_i4			length,
	char			**value)
{
	GSTATUS err = GSTAT_OK;	
	u_i4	len = STlength(name);
	
	if (pout->active_error == TRUE && 
		(STbcompare(name, len, HVAR_STATUS_NUMBER, STlength(HVAR_STATUS_NUMBER), 0) == 0 ||
		STbcompare(name, len, HVAR_STATUS_TEXT, STlength(HVAR_STATUS_TEXT), 0) == 0))
	{
		pout->act_session->error_counter--;
		pout->active_error = FALSE;
	}

	err = WSMGetVariable(
				pout->act_session, 
				name, 
				length, 
				value, 
				WSM_ACTIVE | WSM_USER | WSM_SYSTEM);

	if (err == GSTAT_OK && *value == NULL)
		*value = pout->version20.query.nullvar;

	if (err == GSTAT_OK && *value == NULL)
		err = WSMGetVariable(
					pout->act_session, 
					HVAR_VARNULL, 
					STlength(HVAR_VARNULL), 
					value, 
					WSM_ACTIVE | WSM_USER | WSM_SYSTEM);
return(err); 
}

/*
** Name: WPSHtmlSetInfo() - concat information to a pointer
**
** Description:
**	if the value if a variable name then the value of the variable will be added
**	Allocate memory if necessary
**
** Inputs:
**	char**					: string
**	WPS_HTML_PPARSER_OUT	: out parser structure
**	PPARSER_IN				: in parser structure
**
** Outputs:
**	char**					: string
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
**      14-May-1999 (fanra01)
**          Update test for an empty variable.  If the variable is empty
**          it could be set from a query value so the name needs to be
**          returned and not the empty string.
**      18-Jun-1999 (fanra01)
**          Backout previous change and revert to original behaviour.
**      04-Dec-2000 (fanra01)
**          Length check on variable to ensure we test for a named variable.
*/
GSTATUS
WPSHtmlSetInfo (
    char                    **str,
    WPS_HTML_PPARSER_OUT    pout,
    PPARSER_IN              in)
{
    GSTATUS err = GSTAT_OK;
    u_i4    length;
    u_i4    current = 0;
    char    *value;

    value = in->buffer + in->position;
    length = in->current - in->position;

    if (value[0] == VARIABLE_MARK && length > 0)
    {   /* the name is a variable name */
        char *tmp = NULL;
        err = WPSHtmlGetVar( pout, value + 1, length - 1 , &tmp );
        /*
        ** Revert to original behaviour.  Where empty variables are treated
        ** as such and returned for use.
        ** When using HTML variables and passing values between pages the
        ** variable names should not conflict with column names from
        ** queries as this may cause unpredictable results.
        */
	asct_trace( ASCT_VARS )
	    ( ASCT_TRACE_PARAMS,
	    "Variable %t=%s", length, value,
            ((tmp == NULL) ? "NULL" : ((*tmp == EOS) ? "EOS" : tmp)) );
        if (tmp != NULL)
        {
            value = tmp;
            length = STlength(value);
        }
    }

    if (err == GSTAT_OK && ((value != NULL) || (length > 0)))
    {
        if (*str != NULL)
            current = STlength((*str));

        err = GAlloc((PTR*)str, current + length + 1, TRUE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(value, length, (*str) + current);
            (*str)[current + length] = EOS;
        }
    }
    return(err);
}

/*
** Name: SqlAppend() - append text into the sql statement
**
** Description:
**
** Inputs:
**	WPS_PHTML_QUERY_TEXT: query
**	char*				: text
**	u_i4				: text length
**	bool				: double quote (if TRUE, every quote of the text will be doubled)
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
**      05-Jun-2000 (fanra01)
**          If double quoting requested only doubleup on quotes if there is
**          an odd number.
*/
GSTATUS
SqlAppend (
    WPS_PHTML_QUERY_TEXT query,
    char* str,
    u_i4 length,
    bool doubleQuote)
{
    GSTATUS err = GSTAT_OK;
    u_i4    need = length;

    if (doubleQuote == TRUE)
    {
        u_i4 i;

        need = 0;

        for (i = 0; i < length; i++)
            if (str[i] == '\'')
                need++;
        /*
        ** if the number of quotes is odd then double up
        */
        if ((need & 1) == 0)
        {
            need = length;
            doubleQuote = FALSE;
        }
    }

    need = (((need + query->length + 1) / QUERY_BLOCK_SIZE) + 1) * QUERY_BLOCK_SIZE;

    err = GAlloc(&query->stmt, need, TRUE);
    if (err == GSTAT_OK)
    {
        if (doubleQuote == TRUE)
        {
            u_i4 i;
            for (i = 0; i < length; i++)
            {
                if (str[i] == '\'')
            query->stmt[query->length++] = '\'';
                query->stmt[query->length++] = str[i];
            }
        }
        else
        {
            MECOPY_VAR_MACRO(str, length, query->stmt + query->length);
            query->length += length;
        }
        query->stmt[query->length] = EOS;
    }
return(err);
}

/*
** Name: HtmlAppend() - append text into the html buffer
**
** Description:
**	if variable, it appends the value of the variable.
**
** Inputs:
**	WPS_PHTML_QUERY_TEXT: query
**	char*				: text
**	u_i4				: text length
**	bool				: double quote (if TRUE, every quote of the text will be doubled)
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
HtmlAppend (	
	PPARSER_IN		in, 
	WPS_HTML_PPARSER_OUT pout) 
{
	GSTATUS err = GSTAT_OK;	
	u_i4 length = in->current - in->position;
	char *value = NULL;
	char *name = in->buffer + in->position;

	if (length > 0 && name[0] == VARIABLE_MARK)
		if (name[1] != VARIABLE_MARK)
		{
			err = WPSHtmlGetVar(
							pout,
							name + 1, /* forget the variable mark */
							length - 1,
							&value);
		}
		else
		{
			name++;
			in->current++;
		}

	if (err == GSTAT_OK) 
	{
		if (value == NULL)
			err = WPSBlockAppend(&pout->buffer, name, length);
		else
			err = WPSBlockAppend(&pout->buffer, value, STlength(value));
	}
return(err);
}

/*
** Name: WPSHtmlICE() - ICE tag was found
**
** Description:
**	Initialize the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlICE(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	u_i4 current;
	u_i4 length;

	pout = (WPS_HTML_PPARSER_OUT) out;
	
	if (pout->error != NULL)
		DDFStatusFree(TRC_INFORMATION, &(pout->error));

	pout->format.linksColTab = NULL;
	pout->format.headersColTab = NULL;
	pout->headers = NULL;
	pout->format.tagsColTab = NULL;
	pout->format.firstTag = NULL;
	pout->format.lastTag = NULL;
	pout->version20.query.firstSql = NULL;
	pout->version20.query.dbname = NULL;
	pout->version20.query.user = NULL;
	pout->version20.query.password = NULL;
	pout->version20.query.lastSql = NULL;
	pout->version20.query.nullvar = NULL;
	pout->version20.query.type = HTML_ICE_TYPE_TABLE;
	pout->format.attr = NULL;
	pout->version25.name = NULL;
	pout->version25.transaction = NULL;
	pout->version25.cursor = NULL;
	pout->version25.rows = -1;
	pout->version25.value = NULL;
	pout->version25.repeat = FALSE;
	pout->if_info = NULL;
	pout->if_par_counter = 0;
	/* default value for variable level and equal HTML_ICE_INC_HTML for the include */
	pout->version25.level = WSM_ACTIVE; 

	current = in->current;
	while (in->buffer[current] != '<' && current > in->position)
		current--;

	length = current - in->position;

	pout->error = WPSBlockAppend(pout->result, in->buffer + in->position, length);
	in->position = in->current;
return (GSTAT_OK);
}

/*
** Name: WPSHtmlMoveTo() - move the parser cursor to the current position
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlMoveTo (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	in->position = in->current;
return (GSTAT_OK);
}

/*
** Name: WPSHtmlMoveAfter() - move the parser cursor after the current position
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlMoveAfter (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	in->position = in->current + 1;
return (GSTAT_OK);
}

/*
** Name: WPSHtmlCopy() - copy the text into the HTML buffer 
**						 from the parser cursor to the current position
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlCopy (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		u_i4 length = in->current - in->position;
		pout->error = WPSBlockAppend(pout->result, in->buffer + in->position, length);
		in->position = in->current;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlSql() - add a sql text into the statement list
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlSql (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_QUERY_TEXT query = pout->version20.query.lastSql;
		u_i4 length = in->current - in->position;
		pout->error = SqlAppend(query, in->buffer + in->position, length, FALSE);
		in->position = in->current;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlVar() - add a var component into the HTML buffer
**
** Description:
**	The var component can be a text or a variable
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlVar (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		u_i4 length = in->current - in->position;
		char *value = NULL;
		char *name = in->buffer + in->position;

		if (length > 0 && name[0] == VARIABLE_MARK)
		{
			pout->error = WPSHtmlGetVar(
							pout,
							name + 1, /* forget the variable mark */
							length - 1, 
							&value);
		}

		if (pout->error == GSTAT_OK && length > 0) 
		{
			if (value == NULL)
				pout->error = WPSBlockAppend(&pout->buffer, name, length);
			else
				pout->error = WPSBlockAppend(&pout->buffer, value, STlength(value));
		}
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSqlParam() - add a sql variable component into the statement list
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      15-Sep-98 (marol01)
**          fix sql quote issue. ask sqlAppend method to double quote
**          if value != NULL
**      04-Jan-2000 (fanra01)
**          Bug 99925
**          Test to see if the query is setup before using.
*/
GSTATUS WPSHtmlSqlParam (
    PPARSER_IN  in,
    PPARSER_OUT out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if ((pout->error == GSTAT_OK) &&
        (pout->version20.query.lastSql != NULL))
    {
        WPS_PHTML_QUERY_TEXT query = pout->version20.query.lastSql;
        u_i4 length = in->current - in->position;
        char *value = NULL;
        char *name = in->buffer + in->position;

        if (length > 0 && name[0] == VARIABLE_MARK)
            pout->error = WPSHtmlGetVar(
                pout,
                name + 1, /* forget the variable mark */
                length - 1,
                &value);

        if (pout->error == GSTAT_OK && length > 0)
        {
            if (value == NULL)
                pout->error = SqlAppend(query, name, length, FALSE);
            else
                pout->error = SqlAppend(query, value, STlength(value), TRUE);
        }
        in->position = in->current;
    }
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlNewSqlQuery() - create a new query into the statement list
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**  15-Sep-98 (marol01)
**      deleted redundancy calling WPSHtmlSqlParam.
**  05-June-2000 (fanra01)
**      Test the previous statement, if there is one, to see if it was properly
**      terminated.  If it is not then do not create a new query and continue
**      with the current one.
*/
GSTATUS 
WPSHtmlNewSqlQuery (
    PPARSER_IN  in,
    PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout;
    bool incomplete = FALSE;

    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        WPSHtmlSqlParam (in, out);
        if (pout->error == GSTAT_OK)
        {
            WPS_PHTML_QUERY_TEXT prevstmt = pout->version20.query.lastSql;
            /*
            ** If there is a previous statement and the count for single quote
            ** is odd then the statement is incomplete.
            */
            if (prevstmt != NULL)
            {
                u_i4 i;
                i4 sq = 0;
                char* p = prevstmt->stmt;

                for (i = 0; i < prevstmt->length; i+=1, p+=1)
                {
                    sq += (*p == '\'') ? 1 : 0;
                }
                /*
                ** Odd number of single quotes.  Probably encountered
                ** end of statement character inside a string.
                */
                incomplete =  (sq & 1);
            }
            /*
            ** If the previous statement is incomplete then don't create a new
            ** statement.  The parser should continue and complete the
            ** statement.
            */
            if (!incomplete)
            {
                WPS_PHTML_QUERY_TEXT query;
                pout->error = G_ME_REQ_MEM(0, query, WPS_HTML_QUERY_TEXT, 1);
                if (pout->error == GSTAT_OK)
                {
                    in->position = in->current + 1;
                    if (pout->version20.query.firstSql)
                    {
                        pout->version20.query.lastSql->next = query;
                        pout->version20.query.lastSql = query;
                    }
                    else
                        pout->version20.query.firstSql = query;
                    pout->version20.query.lastSql = query;
                }
            }
        }
    }
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlDatabase() - Set the database name
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlDatabase (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		while (IsSeparator(in->buffer[in->position]))
			in->position++;
		pout->error = WPSHtmlSetInfo(&pout->version20.query.dbname, pout, in);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlAttr() - Set attributes
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlAttr (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		u_i4 length = in->current - in->position;
		pout->error = GAlloc((PTR*)&pout->format.attr, length+1, FALSE);
		if (pout->error == GSTAT_OK) 
		{
			MECOPY_VAR_MACRO(in->buffer + in->position, length, pout->format.attr);
			pout->format.attr[length] = EOS;
			in->position = in->current;
		}
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmllinksName() - Set links name
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
GSTATUS WPSHtmllinksName (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_OVER_TEXT link;

		pout->error = G_ME_REQ_MEM(0, link, WPS_HTML_OVER_TEXT, 1);
		if (pout->error == GSTAT_OK) 
		{
			while (IsSeparator(in->buffer[in->position]))
				in->position++;
			pout->error = WPSHtmlSetInfo(&link->name, pout, in);
			if (pout->error == GSTAT_OK)
			{
				link->next = pout->version20.links;
				pout->version20.links = link;
			}
		}
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmllinksData() - Set links value
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmllinksData (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_OVER_TEXT link;
		link = pout->version20.links;
		if (link != NULL) 
		{
			while (IsSeparator(in->buffer[in->position]))
				in->position++;

			pout->error = WPSHtmlSetInfo(&link->data, pout, in);
		}
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlheadersName() - Set header name
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlheadersName (
	PPARSER_IN		in,
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_OVER_TEXT header;
	
		pout->error = G_ME_REQ_MEM(0, header, WPS_HTML_OVER_TEXT, 1);
		if (pout->error == GSTAT_OK) 
		{
			while (IsSeparator(in->buffer[in->position]))
				in->position++;
			pout->error = WPSHtmlSetInfo(&header->name, pout, in);
			if (pout->error == GSTAT_OK)
			{
				header->next = pout->headers;
				pout->headers = header;
			}
		}
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlheadersData() - Set header value
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlheadersData (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_OVER_TEXT header;
	
		header = pout->headers;
		if (header != NULL) 
		{
			while (IsSeparator(in->buffer[in->position]))
				in->position++;
		
			pout->error = WPSHtmlSetInfo(&header->data, pout, in);
		}
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlHtmlVar() - Set var inside the HTML macro tag
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlHtmlVar (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_TAG_TEXT tag;
	
		pout->error = G_ME_REQ_MEM(0, tag, WPS_HTML_TAG_TEXT, 1);
		if (pout->error == GSTAT_OK) 
		{
			pout->error = WPSHtmlSetInfo(&tag->name, pout, in);
			if (pout->error == GSTAT_OK)
			{
				tag->type = HTML_ICE_HTML_VAR;
				if (pout->format.firstTag == NULL)
					pout->format.firstTag = pout->format.lastTag = tag;
				else
					pout->format.lastTag = pout->format.lastTag->next = tag;
			}
		}
		in->position = in->current;
		if (in->buffer[in->position] == '`')
			in->position++;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlHtmlText() - Set text inside the HTML macro tag
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlHtmlText (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		WPS_PHTML_TAG_TEXT tag = NULL;
	
		if (in->current > in->position)
		{
			pout->error = G_ME_REQ_MEM(0, tag, WPS_HTML_TAG_TEXT, 1);
			if (pout->error == GSTAT_OK) 
			{
				pout->error = WPSHtmlSetInfo(&tag->name, pout, in);
				if (pout->error == GSTAT_OK)
				{
					tag->type = HTML_ICE_HTML_TEXT;
					if (pout->format.firstTag == NULL)
						pout->format.firstTag = pout->format.lastTag = tag;
					else
						pout->format.lastTag = pout->format.lastTag->next = tag;
				}
			}
		}
		in->position = in->current;
		if (in->buffer[in->position] == '`')
			in->position++;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlTypeLink() - Set macro type with LINK
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeLink (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_LINK;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeOList() - Set macro type with OLIST
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeOList (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_OLIST;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeUList() - Set macro type with ULIST
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeUList (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_ULIST;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeCombo() - Set macro type with COMBO
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeCombo (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_COMBO;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeTable() - Set macro type with TABLE
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeTable (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_TABLE;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeImage() - Set macro type with IMAGE
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeImage (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_IMAGE;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTypeRaw() - Set macro type with RAW
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTypeRaw (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version20.query.type = HTML_ICE_TYPE_RAW;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlUser() - Set the user name
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlUser(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		while (IsSeparator(in->buffer[in->position]))
			in->position++;

		pout->error = WPSHtmlSetInfo(&pout->version20.query.user, pout, in);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlPassword() - Set the user password
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlPassword(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		while (IsSeparator(in->buffer[in->position]))
			in->position++;

		pout->error = WPSHtmlSetInfo(&pout->version20.query.password, pout, in);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlExtension() - Set the blob extension
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      15-Apr-1999 (fanra01)
**          Remove extension set from previous query on the same page.
**          Otherwise the new extension is appended to the previous.
*/
GSTATUS 
WPSHtmlExtension(
    PPARSER_IN  in,
    PPARSER_OUT out)
{
    WPS_HTML_PPARSER_OUT pout;
    char* ext;

    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        if ((ext = pout->version20.query.extension) != NULL)
        {
            MEfree(ext);
            pout->version20.query.extension = NULL;
        }
        pout->error = WPSHtmlSetInfo(&pout->version20.query.extension, pout, in);
        in->position = in->current + 1;
    }
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlNullVar() - Set the nullvar value
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlNullVar(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;

	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version20.query.nullvar, pout, in);
		in->position = in->current + 1;
	}

return(GSTAT_OK); 
}

/*
** Name: WPSHtmlInitializeMacro() - Initialize a database execution
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	WPS_HTML_PPARSER_OUT	out
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
**  10-Sep-98 (marol01) (scott paley request)
**      added HVAR_DATABASE as a default database name.
**			if the database name doesn't set, ICE checks if HVAR_DATABASE exists
**			then, if the database name doesn't set, ICE checks config.dat
*/
GSTATUS 
WPSHtmlInitializeMacro (
	WPS_HTML_PPARSER_OUT	pout,
	PPARSER_IN		in)
{
	GSTATUS			err = GSTAT_OK;	
	char				szConfig [CONF_STR_SIZE];
	char				*database = NULL;

	if (pout->version20.query.dbname == NULL)
	{
			err = WSMGetVariable (
								pout->act_session, 
								HVAR_DATABASE, 
								STlength(HVAR_DATABASE), 
								&database, 
								WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

			if (err == GSTAT_OK && database == NULL)
			{
				STprintf (szConfig, CONF_DEFAULT_DATABASE, PMhost());
				PMget( szConfig, &database);
			}
		
			if (err == GSTAT_OK)
				if (database != NULL)
				{
					u_i4 length = STlength(database) + 1;
					err = G_ME_REQ_MEM(0, pout->version20.query.dbname, char, length);
					if (err == GSTAT_OK) 
						MECOPY_VAR_MACRO(database, length, pout->version20.query.dbname);
				}
				else
					err = DDFStatusAlloc(E_WS0045_UNDEF_DBNAME);
	}
	else if (pout->act_session->user_session == NULL)
	{
		char* overwrite;
		STprintf (szConfig, CONF_ALLOW_DB_OVR, PMhost());
		if (PMget( szConfig, &overwrite) != OK ||
				overwrite == NULL ||
				STcompare(overwrite, "ON") != 0)
			err = DDFStatusAlloc(E_WS0045_UNDEF_DBNAME);
	}

	if (err == GSTAT_OK) 
		err = DriverLoad(OPEN_INGRES_DLL, &pout->driver);
return(GSTAT_OK);
}

/*
** Name: WPSHtmlColTabs() - Define the correlation between header/link tables
**							and column names
**
** Description:
**
** Inputs:
**	USR_PTRANS			current
**	WPS_HTML_PPARSER_OUT	out
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
WPSHtmlColTabs (
	u_i4				dbtype,
	DB_QUERY			*query,
	WPS_HTML_PPARSER_OUT	pout)
{
	GSTATUS			err = GSTAT_OK;	
	u_i4			i =0;
	char			*data = NULL;
	char			Coltype;
	u_i4			numberOfCols;
	WPS_PHTML_OVER_TEXT over;
	WPS_PHTML_TAG_TEXT tag;

	numberOfCols = pout->version20.query.nb_of_columns;

	if (pout->version20.links != NULL)
		err = G_ME_REQ_MEM(
				0, 
				pout->format.linksColTab, 
				WPS_PHTML_OVER_TEXT, 
				numberOfCols);

	if (err == GSTAT_OK && pout->headers != NULL)
		err = G_ME_REQ_MEM(
				0, 
				pout->format.headersColTab, 
				WPS_PHTML_OVER_TEXT, 
				numberOfCols);

	if (err == GSTAT_OK && pout->format.firstTag != NULL)
		err = G_ME_REQ_MEM(
				0, 
				pout->format.tagsColTab, 
				char*, 
				numberOfCols);
		
	for (i = 0; err == GSTAT_OK && i < numberOfCols; i++) 
	{
		err = DBName(dbtype)(query, i+1, &data);
		if (err == GSTAT_OK)
			err = DBType(dbtype)(query, i+1, &Coltype);

		for (over = pout->version20.links; err == GSTAT_OK && over != NULL; over = over->next) 
			if (over->name != NULL && STcompare(data, over->name) == 0) 
			{
				pout->format.linksColTab[i] = over;
				break;
			}
		for (over = pout->headers; err == GSTAT_OK && over != NULL; over = over->next) 
			if (over->name != NULL && STcompare(data, over->name) == 0) 
			{
				pout->format.headersColTab[i] = over;
				break;
			}
		for (tag = pout->format.firstTag; err == GSTAT_OK && tag != NULL; tag = tag->next) 
			if (tag->type == HTML_ICE_HTML_VAR && 
				tag->name != NULL &&
				STcompare(data, tag->name + 1) == 0) 
			{
				if (Coltype == DDF_DB_BLOB)
					tag->type |= HTML_ICE_HTML_BINARY;
				tag->position = i+1;
			}
	}

	tag = pout->format.firstTag;
	while (tag != NULL) 
	{	/* check unknown variable name. In this way the variable becomes a text */
		if (tag->position == 0)
			tag->type = HTML_ICE_HTML_TEXT;
		tag = tag->next;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlGetColumnName() - Execute one database statement and format the result
**
** Description:
**
** Inputs:
**	u_i4					: database number
**	DB_QUERY				: query
**	u_nat*					: number of rows expected
**	WPS_HTML_PPARSER_OUT	: pout
**
** Outputs:
**	u_nat*					: number of rows processed
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
WPSHtmlGetColumnName (
	u_i4				dbtype,
	DB_QUERY			*query,
	u_i4				*maximum,
	WPS_HTML_PPARSER_OUT	pout)
{
	GSTATUS			err = GSTAT_OK;	
	u_i4			numberOfCols =0; 
	u_i4			i =0;
	u_i4			type = 0;
	char*			data = NULL;
	u_i4			more = FALSE;
	u_i4			count = 0;

	type = pout->version20.query.type;

	err = DBNumberOfProperties(dbtype)(query, &numberOfCols);
	pout->version20.query.nb_of_columns = numberOfCols;

	if (err == GSTAT_OK && formatterTab[type].OutputStart)
		err = formatterTab[type].OutputStart (&pout->format, &pout->buffer);

	if (err == GSTAT_OK && numberOfCols) 
	{
		if (formatterTab[type].RowStart != NULL)
			err = formatterTab[type].RowStart (&pout->format, &pout->buffer, TRUE);

		if (err == GSTAT_OK)
		{
			WPS_PHTML_OVER_TEXT over;
			WPS_PHTML_TAG_TEXT tag;

			if (pout->version20.links != NULL)
				err = G_ME_REQ_MEM(
						0, 
						pout->format.linksColTab, 
						WPS_PHTML_OVER_TEXT, 
						numberOfCols);

			if (err == GSTAT_OK && pout->headers != NULL)
				err = G_ME_REQ_MEM(
						0, 
						pout->format.headersColTab, 
						WPS_PHTML_OVER_TEXT, 
						numberOfCols);

			if (err == GSTAT_OK && pout->format.firstTag != NULL)
				err = G_ME_REQ_MEM(
						0, 
						pout->format.tagsColTab, 
						char*, 
						numberOfCols);
		
			for (over = pout->version20.links; err == GSTAT_OK && over != NULL; over = over->next) 
				if (over->name != NULL && STcompare(WSF_COLUMN_NAME, over->name) == 0) 
				{
					pout->format.linksColTab[i] = over;
					break;
				}
			for (over = pout->headers; err == GSTAT_OK && over != NULL; over = over->next) 
				if (over->name != NULL && STcompare(WSF_COLUMN_NAME, over->name) == 0) 
				{
					pout->format.headersColTab[i] = over;
					break;
				}
			for (tag = pout->format.firstTag; err == GSTAT_OK && tag != NULL; tag = tag->next) 
				if (tag->type == HTML_ICE_HTML_VAR && 
					tag->name != NULL &&
					STcompare(WSF_COLUMN_NAME, tag->name + 1) == 0) 
				{
					tag->position = 1;
				}

			if (err == GSTAT_OK)
			{
				tag = pout->format.firstTag;
				while (tag != NULL) 
				{	/* check unknown variable name. In this way the variable becomes a text */
					if (tag->position == 0)
						tag->type = HTML_ICE_HTML_TEXT;
					tag = tag->next;
				}	
			}
		}

		if (err == GSTAT_OK && formatterTab[type].TextItem != NULL)
			err = formatterTab[type].TextItem (&pout->format, &pout->buffer, 0, WSF_COLUMN_NAME, TRUE);
		
		if (err == GSTAT_OK && formatterTab[type].RowEnd != NULL)
			err = formatterTab[type].RowEnd (&pout->format, &pout->buffer, TRUE);

		for (i = 1; err == GSTAT_OK && i <= numberOfCols; i++) 
		{
			if (formatterTab[type].RowStart != NULL)
				err = formatterTab[type].RowStart (&pout->format, &pout->buffer, FALSE);

			err = DBName(dbtype)(query, i, &data);
			if (err == GSTAT_OK && formatterTab[type].TextItem != NULL)
				err = formatterTab[type].TextItem (&pout->format, &pout->buffer, 1, data, FALSE);

			if (err == GSTAT_OK && formatterTab[type].RowEnd != NULL)
				err = formatterTab[type].RowEnd (&pout->format, &pout->buffer, FALSE);
			count++;
		}
		
		if (err == GSTAT_OK && formatterTab[type].OutputEnd != NULL)
			err = formatterTab[type].OutputEnd (&pout->format, &pout->buffer);
	}
	*maximum = count;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSingleQuery() - Execute one database statement and format the result
**
** Description:
**
** Inputs:
**	u_i4					: database number
**	DB_QUERY				: query
**	u_nat*					: number of rows expected
**	WPS_HTML_PPARSER_OUT	: pout
**
** Outputs:
**	u_nat*					: number of rows processed
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
**      20-Oct-98 (fanra01)
**          Remove trailing spaces from text columns.
**      11-Mar-1999 (fanra01)
**          Don't attempt trim if data is null.
**      17-Mar-1999 (fanra01)
**          Remove additional check for formating NULL data as function
**          should deal with NULL values.
**      06-Apr-1999 (fanra01)
**          For blobs use the url path if there is one otherwise use the
**          os path.
**      12-Apr-1999 (fanra01)
**          Trim all trailing blanks from converted data.
**      15-Apr-1999 (fanra01)
**          Set the data value to null before retrieving. In the case of blobs
**          the value is stored and not returned and the previous value
**          remains.  Add a check in the blob case to test for null data value.
**          If no filename has been returned or if the extension is unknown
**          attempt to set it from the EXT option.
**      28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSrequest.
**      20-Mar-2002 (fanra01)
**          For each retrieval of data that gets built into the output page
**          free the buffer after the copy is made.
*/
GSTATUS
WPSHtmlSingleQuery (
    u_i4                   dbtype,
    DB_QUERY                *query,
    u_i4                   *maximum,
    WPS_HTML_PPARSER_OUT    pout)
{
    GSTATUS err = GSTAT_OK;
    u_i4   numberOfCols =0;
    u_i4   i =0;
    u_i4   type = 0;
    char    Coltype;
    char*   data = NULL;
    bool    more = FALSE;
    u_i4   count = 0;
    i4      trimlen;

    type = pout->version20.query.type;

    err = DBNumberOfProperties(dbtype)(query, &numberOfCols);
    pout->version20.query.nb_of_columns = numberOfCols;

    if (err == GSTAT_OK && numberOfCols > 0)
    {
        if (formatterTab[type].OutputStart)
        {
            err = formatterTab[type].OutputStart (&pout->format,
                &pout->buffer);
        }
        if (err == GSTAT_OK)
        {
            /*
            ** if this is not an XML request process as usual
            ** otherwise build a column name header list
            */
            if ((pout->version20.query.type != XML_ICE_TYPE_XML) &&
                (pout->version20.query.type != XML_ICE_TYPE_XML_PDATA))
            {
                if (err == GSTAT_OK)
                {
                    err = WPSHtmlColTabs (dbtype, query, pout);
                }
                if (formatterTab[type].RowStart != NULL)
                {
                    err = formatterTab[type].RowStart (&pout->format,
                        &pout->buffer, TRUE);
                }
                for (i = 1; err == GSTAT_OK && i <= numberOfCols; i++)
                {
                    data = NULL;
                    err = DBName(dbtype)(query, i, &data);
                    if (err == GSTAT_OK && formatterTab[type].TextItem != NULL)
                    {
                        err = formatterTab[type].TextItem (&pout->format,
                            &pout->buffer, i, data, TRUE);
                    }
                    if (data != NULL)
                    {
                        MEfree( data );
                    }
                }

                if (err == GSTAT_OK && formatterTab[type].RowEnd != NULL)
                {
                    err = formatterTab[type].RowEnd (&pout->format, &pout->buffer,
                        TRUE);
                }
            }
            else
            {
                WPS_PHTML_OVER_TEXT header;

                if (pout->headers == NULL)
                {
                    for (i = 1; err == GSTAT_OK && i <= numberOfCols; i++)
                    {
                        data = NULL;
                        err = DBName(dbtype)(query, i, &data);
                        if (err == GSTAT_OK && data != NULL)
                        {
                            err = G_ME_REQ_MEM(0, header, WPS_HTML_OVER_TEXT, 1);
                            if (err == GSTAT_OK)
                            {
                                header->name = STalloc(data);
                                header->data = STalloc(data);

                                header->next = pout->headers;
                                pout->headers = header;
                            }
                        }
                        if (data != NULL)
                        {
                            MEfree( data );
                        }
                    }
                }
                if (err == GSTAT_OK)
                {
                    err = WPSHtmlColTabs (dbtype, query, pout);
                }
             }

            if (err == GSTAT_OK)
                err = DBNext(dbtype)(query, &more);

            while (err == GSTAT_OK && more == TRUE)
            {
                if (formatterTab[type].RowStart != NULL)
                {
                    err = formatterTab[type].RowStart (&pout->format,
                        &pout->buffer, FALSE);
                }
                for (i = 1; err == GSTAT_OK && i <=    numberOfCols; i++)
                {
                    data = NULL;
                    err = DBText(dbtype)(query, i, &data);
                    if (err == GSTAT_OK)
                    {
                        err = DBType(dbtype)(query, i, &Coltype);
                    }
                    if (err == GSTAT_OK)
                    {
                        switch (Coltype)
                        {
                            case DDF_DB_BLOB :
                                if ( formatterTab[type].BinaryItem != NULL )
                                {
                                    char        *prefix = data;
                                    char        *suffix = data;
                                    char        *dirloc = NULL;
                                    WPS_FILE    *cache = NULL;
                                    bool        status;
                                    char        sep=CHAR_FILE_SUFFIX_SEPARATOR;

                                    if (data != NULL)
                                    {
                                        while ( suffix[0] != EOS &&
                                            suffix[0] != sep)
                                        suffix++;
                                        if (suffix[0] == sep)
                                        {
                                            suffix[0] = EOS;
                                            suffix++;
                                        }
                                    }
                                    else
                                    {
                                        if (pout->version20.query.extension != NULL)
                                        {
                                            suffix = pout->version20.query.extension;
                                        }
                                    }
                                    if ((err=WSMGetVariable( pout->act_session,
                                        HVAR_OUTPUT_DIR_LABEL,
                                        STlength( HVAR_OUTPUT_DIR_LABEL ),
                                        &dirloc, WSM_USER)) == GSTAT_OK)
                                    {
                                        err = WPSRequest(
                                            (pout->act_session->user_session != NULL) ?
                                            pout->act_session->user_session->name : NULL,
                                            pout->act_session->scheme,
                                            pout->act_session->host,
                                            pout->act_session->port,
                                            dirloc,
                                            0,
                                            prefix,
                                            suffix,
                                            &cache,
                                            &status);
                                    }
                                    if (err == GSTAT_OK)
                                    {
                                        err = DBBlob(dbtype)(query, i,
                                            &cache->file->loc);
                                        if (err == GSTAT_OK)
                                        {
                                            char* pathref;
                                            pathref = (cache->url != NULL) ?
                                                cache->url :
                                                cache->file->info->path;

                                            err = formatterTab[type].BinaryItem (
                                                &pout->format,
                                                &pout->buffer,
                                                i,
                                                pathref,
                                                FALSE);
                                       }
                                       CLEAR_STATUS(WCSLoaded(cache->file, TRUE))
                                       CLEAR_STATUS(WPSRelease(&cache));
                                    }
                                }
                                break;

                            case DDF_DB_TEXT:
                            default :
                                /*
                                ** Remove trailing blanks returned from all
                                ** columns
                                */
                                if (data != NULL)
                                {
                                    trimlen = STtrmwhite (data);
                                }
                                /* No need to check for data NULL as function
                                ** should.
                                */
                                if (formatterTab[type].TextItem != NULL)
                                {
                                    err = formatterTab[type].TextItem (
                                        &pout->format, &pout->buffer, i,
                                        data, FALSE);
                                }
                        }
                    }
                    if (data != NULL)
                    {
                        MEfree( data );
                    }
                }
                if (err == GSTAT_OK && formatterTab[type].RowEnd != NULL)
                {
                    err = formatterTab[type].RowEnd (&pout->format,
                        &pout->buffer, FALSE);
                }
                if (err == GSTAT_OK)
                {
                    if ((*maximum) > 0 && ++count >= (*maximum))
                        more = FALSE;
                    else
                        err = DBNext(dbtype)(query, &more);
                }
            }
            if (err == GSTAT_OK && formatterTab[type].OutputEnd != NULL)
            {
                err = formatterTab[type].OutputEnd (&pout->format,
                    &pout->buffer);
            }
        }
    }
    *maximum = count;
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlExecute() - Execute multiple database statements and format the result
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlExecute (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
	USR_PTRANS	transaction = NULL;
	u_i4		    count = 0;
	WPS_PHTML_QUERY_TEXT stmt = pout->version20.query.firstSql;
	
	in->position = in->current + 1;

	pout->error = WPSHtmlInitializeMacro(pout, in);
	if (pout->error == GSTAT_OK) 
	{
	    if (stmt != NULL)
	    {
		if (pout->version25.transaction != NULL && 
		    pout->version25.transaction[0] != EOS)
		    pout->error = WSMGetTransaction(
						    pout->act_session->user_session, 
						    pout->version25.transaction, 
						    &transaction);

		if (pout->error == GSTAT_OK && transaction == NULL)
		    pout->error = WSMCreateTransaction(
						       pout->act_session->user_session, 
						       pout->driver, 
						       pout->version20.query.dbname, 
						       pout->version25.transaction, 
						       pout->version20.query.user,
						       pout->version20.query.password, 
						       &transaction);

		if (pout->error == GSTAT_OK) 
		{
		    USR_PCURSOR cursor = NULL;
		    char *cursor_name;

		    while (pout->error == GSTAT_OK && stmt != NULL) 
		    {
			count = pout->version25.rows;

			if (stmt->next == NULL)
			    cursor_name = pout->version25.cursor;
			else
			    cursor_name = NULL;

			if (pout->version25.cursor != NULL && 
			    pout->version25.cursor[0] != EOS)
			    pout->error = WSMGetCursor(
						       transaction, 
						       cursor_name, 
						       &cursor);

			if (pout->error == GSTAT_OK && cursor == NULL)
			    pout->error = WSMCreateCursor(
							  transaction,
							  cursor_name, 
							  stmt->stmt,
							  &cursor);

			if (pout->error == GSTAT_OK)
			    if (count == 0)
				pout->error = WPSHtmlGetColumnName(
								   transaction->session->driver, 
								   &cursor->query, 
								   &count,
								   pout);
			    else
				pout->error = WPSHtmlSingleQuery(
								 transaction->session->driver, 
								 &cursor->query, 
								 &count,
								 pout);
						
			if (cursor->name == NULL)
			{
			    CLEAR_STATUS(WSMDestroyCursor(
							  cursor,
							  &count, 
							  FALSE));
			    cursor = NULL;
			}
			stmt = stmt->next;
		    }

		    if (pout->error == GSTAT_OK)
		    {
			char tmp[NUM_MAX_INT_STR];
			CVna(count, tmp);
			pout->error = WSMAddVariable (
						      pout->act_session, 
						      HVAR_ROWCOUNT,
						      STlength(HVAR_ROWCOUNT),
						      tmp,
						      STlength(tmp),
						      WSM_ACTIVE);
		    }

		    if (transaction->trname == NULL)
		    {
			if (pout->error == GSTAT_OK)
			    pout->error = WSMCommit(transaction, FALSE);
			else
			    CLEAR_STATUS(WSMRollback(transaction, FALSE));
		    }
		}
	    }
	}
    }
    return(GSTAT_OK); 
}

/*
** Name: WPSHtmlName25() - Initialize name for 2.5 features
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlName25(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version25.name, pout, in);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlValue25() - Initialize value for 2.5 features
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlValue25(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version25.value, pout, in);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSAppendCharToValue
**
** Description:
**      Takes the current character an appends it to the variable value.
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      01-Dec-2000 (fanra01)
**          Bug 102857
**          Created.
*/
GSTATUS 
WPSAppendCharToValue( PPARSER_IN in, PPARSER_OUT out )
{
    GSTATUS err = GSTAT_OK;
    WPS_HTML_PPARSER_OUT pout = (WPS_HTML_PPARSER_OUT) out;
    u_i4    length = 0;
    u_i4    current = 0;
    char    value[2];
    char**  str = &pout->version25.value;

    CMcpychar( (in->buffer + in->position), value );
    CMbyteinc( length, value );

    if (err == GSTAT_OK)
    {
        if (*str != NULL)
            current = STlength(*str);

        err = GAlloc((PTR *)str, current + length + 1, TRUE);
        if (err == GSTAT_OK)
        {
            MECOPY_VAR_MACRO(value, length, (*str) + current);
            (*str)[current + length] = EOS;
        }
        in->position += (length * 2);
    }
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlIncHTML() - Set the include type to HTML
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIncHTML(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;

	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version25.level = HTML_ICE_INC_HTML;
	in->position = in->current + 1;
return(GSTAT_OK);
}

/*
** Name: WPSHtmlIncReport() - Set the include type to REPORT
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIncReport(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;

	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version25.level = HTML_ICE_INC_REPORT;
	in->position = in->current + 1;
return(GSTAT_OK);
}

/*
** Name: WPSHtmlIncExe() - Set the include type to EXECUTABLE
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIncExe(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;

	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version25.level = HTML_ICE_INC_EXE;
	in->position = in->current + 1;
return(GSTAT_OK);
}

/*
** Name: WPSHtmlIncMulti() - Set the include type to MULTIMEDIA
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIncMulti(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;

	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version25.level = HTML_ICE_INC_MULTI;
	in->position = in->current + 1;
return(GSTAT_OK);
}

/*
** Name: WPSHtmlInclude() - Execute the include macro
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**	10-Sep-98 (marol01)
**		Moved the document/unit control to wcsdoc.c
**		changed the function to generate a warning the error is 
**		E_WS0073_WCS_BAD_DOCUMENT
**  18-Nov-1998 (fanra01)
**      Add memory free for read buffer.    
**  10-Mar-1999 (fanra01)
**      Add check for external facet and read message into buffer if its not
**      available as a url.
**  28-Apr-2000 (fanra01)
**          Add additional scheme parameter to WPSrequest.
**  10-Oct-2000 (fanra01)
**          Allow include of page in 2.0 mode.
**  20-Mar-2001 (fanra01)
**          Correct function spelling of WCSDispatchName.
*/
GSTATUS 
WPSHtmlInclude(
    PPARSER_IN  in,
    PPARSER_OUT out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        in->position = in->current + 1;

        pout->error = ParseHTMLVariables(
                         pout->act_session,
                         pout->version25.value);

        if (pout->error == GSTAT_OK)
        {
            switch (pout->version25.level)
            {
                case HTML_ICE_INC_EXE:
                case HTML_ICE_INC_REPORT:
                {
                    u_i4 length = STlength(pout->version25.name);

                    if (pout->version25.level == HTML_ICE_INC_EXE)
                    pout->error = WSMAddVariable (
                        pout->act_session,
                        HVAR_APPLICATION,
                        STlength(HVAR_APPLICATION),
                        pout->version25.name,
                        length,
                        WSM_ACTIVE);
                    else
                    pout->error = WSMAddVariable (
                        pout->act_session,
                        HVAR_REPORT_NAME,
                        STlength(HVAR_REPORT_NAME),
                        pout->version25.name,
                        length,
                        WSM_ACTIVE);

                    if (pout->error == GSTAT_OK)
                        pout->error = WPSPerformVariable (
                            pout->act_session,
                            pout->result);
                }
                break;
                default:
                {
                    char *unit = NULL;
                    char *location = NULL;
                    char *name = NULL;
                    char *suffix = NULL;
                    bool access;

                    pout->error = WCSDispatchName(
                                  pout->version25.name,
                                  &unit,
                                  &location,
                                  &name,
                                  &suffix);

                    if (pout->error == GSTAT_OK &&
                    (unit == NULL || unit[0] == EOS))
                    {
                        char *def_unit = NULL;
                        pout->error = WSMGetVariable (
                            pout->act_session,
                            HVAR_UNIT,
                            STlength(HVAR_UNIT),
                            &def_unit,
                            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
                        if (def_unit != NULL)
                            unit = def_unit;
                    }

                    if ((pout->error == GSTAT_OK) &&
                        (name != NULL && name[0] != EOS))
                    {
                        bool        status;
                        u_i4        unit_id;
                        WPS_PFILE    cache;

                        pout->error = WCSGetUnitId(unit, &unit_id);
                        if (pout->error == GSTAT_OK)
                        {
                            pout->error = WPSRequest(
                                (pout->act_session->user_session != NULL) ?
                                pout->act_session->user_session->name : NULL,
                                pout->act_session->scheme,
                                pout->act_session->host,
                                pout->act_session->port,
                                location,
                                unit_id,
                                name,
                                suffix,
                                &cache,
                                &status);

                            if (pout->error == GSTAT_OK)
                            {
                                bool load = FALSE;
                                bool page;
                                u_i4 authorization = WSF_EXE_RIGHT;

                                pout->error = WCSDocCheckFlag (cache->file->doc_id, WSF_PAGE, &page);

                                if ((pout->act_session->user_session == NULL) && (cache != NULL))
                                {
                                    page = TRUE;
                                }
                                if (pout->version25.repeat == TRUE && page == FALSE)
                                pout->error = DDFStatusAlloc (E_WS0085_WSF_NO_REPEAT);

                                if (pout->version25.repeat == FALSE && page == TRUE)
                                authorization = WSF_READ_RIGHT;

                                if (pout->error == GSTAT_OK)
                                pout->error = WSSDocAccessibility(
                                    pout->act_session->user_session,
                                    cache->file->doc_id,
                                    authorization,
                                    &access);

                                if (pout->error == GSTAT_OK)
                                if (access == FALSE)
                                {
                                    pout->error = DDFStatusAlloc( E_WS0065_WSF_UNAUTH_ACCESS );
                                    CLEAR_STATUS(WPSSetError (pout, ERROR_LVL_WARNING));
                                }
                                else
                                {
                                    if (status == WCS_ACT_LOAD)
                                    {
                                    bool is_external = TRUE;
                                    if (cache->file->doc_id != SYSTEM_ID)
                                        pout->error = WCSDocCheckFlag (cache->file->doc_id, WCS_EXTERNAL, &is_external);

                                    if (is_external == FALSE)
                                        pout->error = WCSExtractDocument (cache->file);

                                    if (pout->error == GSTAT_OK)
                                        load = TRUE;
                                    }

                                    if (pout->error == GSTAT_OK)
                                    {
                                    if (page == FALSE)
                                    {
                                        if (cache->url != NULL)
                                        {
                                            pout->error = WPSBlockAppend(
                                                     &pout->buffer,
                                                     cache->url,
                                                     STlength(cache->url));
                                        }
                                        else
                                        {
                                            char* locspec = NULL;
                                            LOtos (&cache->file->loc, &locspec);
                                            pout->error = DDFStatusAlloc (E_WS0095_WPS_INVALID_FACET_REF);
                                            DDFStatusInfo (pout->error, "%s", locspec);
                                        }
                                    }
                                    else
                                    {
                                        pout->error = WCSOpen (cache->file, "r", SI_BIN, 0);
                                        if (pout->error == GSTAT_OK)
                                        {
                                        char *tmp;
                                        pout->error = G_ME_REQ_MEM(0, tmp, char, WSF_READING_BLOCK);
                                        if (pout->error == GSTAT_OK)
                                        {
                                            i4 read;
                                            do
                                            {
                                            pout->error = WCSRead(cache->file,
                                                          WSF_READING_BLOCK,
                                                          &read,
                                                          tmp);

                                            if (pout->error == GSTAT_OK &&
                                                read > 0)
                                                pout->error = WPSBlockAppend(
                                                             &pout->buffer,
                                                             tmp,
                                                             read);
                                            }
                                            while (pout->error == GSTAT_OK && read > 0);
                                            MEfree (tmp);
                                        }
                                        CLEAR_STATUS( WCSClose(cache->file) );
                                        }
                                    }
                                    }
                                }
                                if (status == WCS_ACT_LOAD)
                                CLEAR_STATUS(WCSLoaded(cache->file, load));

                                CLEAR_STATUS(WPSRelease(&cache));
                            }
                            else
                            {
                                if (pout->error->number == E_WS0073_WCS_BAD_DOCUMENT)
                                CLEAR_STATUS(WPSSetError (pout, ERROR_LVL_WARNING));
                            }
                        }
                    }
                }
            }
        }
    }
    return(GSTAT_OK); 
}

/*
** Name: WPSHtmlDeclLevel() - Set the Declaration variable level
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      30-Oct-2001 (fanra01)
**          Replace ambiguous profile error message with authorization
**          and permission messages.
**      30-Oct-2001 (fanra01)
**          Error identifier name changed because of truncation when message
**          file compiled.
*/
GSTATUS 
WPSHtmlDeclLevel(
    PPARSER_IN        in,
    PPARSER_OUT        out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        u_i4 length = in->current - in->position;
        char* level = NULL;

        pout->error = G_ME_REQ_MEM(0, level, char, length + 1);
        if (pout->error == GSTAT_OK)
        {
            pout->version25.level = 0;
            MECOPY_VAR_MACRO(in->buffer + in->position, length, level);
            level[length] = EOS;
            CVlower(level);
            if (STcompare(level, STR_DECL_SERVER) == 0)
            {
                if (pout->act_session->user_session == NULL)
                    pout->error = DDFStatusAlloc( E_WS0101_WSS_UNAUTHORIZED_USE );

                if (pout->act_session->user_session->userid != SYSTEM_ID)
                {
                    bool    profile;
                    pout->error = DDSCheckUserFlag (
                            pout->act_session->user_session->userid,
                            WSF_USR_ADM,
                            &profile);
                    if (profile == FALSE)
                        pout->error = DDFStatusAlloc( E_WS0102_WSS_INSUFFICIENT_PERM );
                }
                pout->version25.level = WSM_SYSTEM;
            }
            if (pout->error == GSTAT_OK && STcompare(level, STR_DECL_SESSION) == 0)
                pout->version25.level = WSM_USER;
            if (pout->error == GSTAT_OK && STcompare(level, STR_DECL_PAGE) == 0)
                pout->version25.level = WSM_ACTIVE;
            MEfree((PTR)level);
        }
        if (pout->error == GSTAT_OK && pout->version25.level == 0)
            pout->error = DDFStatusAlloc( E_WS0056_WPS_BAD_DECL_LVL );

        in->position = in->current + 1;
    }
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlDecl() - Declare a macro variable
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      23-Jan-2001 (hanje04)
**          Added Null check on pout->version25.value when calling
**          WSMAddVariable because STlength(NULL) will SEGV if STlength
**          is defined to be strlen(). For pout->version25.value == NULL,
**          we now pass 0.
**      20-Aug-2003 (fanra01)
**          Append parsed buffer to the output error message.
*/
GSTATUS 
WPSHtmlDecl(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
	in->position = in->current + 1;

	if (pout->version25.name != NULL && pout->version25.name[0] != EOS)
	{
	    if (pout->version25.repeat == TRUE && 
		pout->version25.value != NULL && 
		pout->version25.value[0] != EOS)
	    {
		WPS_PBUFFER		temp_buffer;
		pout->error = G_ME_REQ_MEM(0, temp_buffer, WPS_BUFFER, 1);
		if (pout->error == GSTAT_OK)
		{
		    pout->error	= WPSBlockInitialize(temp_buffer);
		    if (pout->error == GSTAT_OK)
		    {
			u_i4	length = STlength(pout->version25.value);
			WPS_HTML_PPARSER_OUT out;
			PARSER_IN in;

			MEfill (sizeof (PARSER_IN), 0, (PTR)&in);
                        in.first_node = HTML_FIRST_NODE;
                        in.buffer = pout->version25.value;
                        in.length = length;
                        in.size = length;

			pout->error = G_ME_REQ_MEM(0, out, WPS_HTML_PARSER_OUT, 1);
			if (pout->error == GSTAT_OK) 
			{
			    out->act_session = pout->act_session;
			    out->result = temp_buffer;
			    out->active_error = pout->active_error;
			    pout->error	= WPSBlockInitialize(&out->buffer);
			    if (pout->error == GSTAT_OK)
			    {
				pout->error = ParseOpen(html, &in, (PPARSER_OUT) out);
				if (pout->error == GSTAT_OK)
				    pout->error = Parse(html, &in, (PPARSER_OUT) out);
				if (pout->error == GSTAT_OK)
				    pout->error = ParseClose(html, &in, (PPARSER_OUT) out);
                /*
                ** Add parsed string to output message on error
                */
                if (pout->error != GSTAT_OK)
                {
                    DDFStatusInfo( pout->error, "%s\n", pout->version25.value );
                }
				pout->active_error = out->active_error;
				CLEAR_STATUS( WPSFreeBuffer(&out->buffer) );
			    }
			}
			MEfree((PTR)out);

			if (pout->error == GSTAT_OK)
			    pout->error = WSMAddVariable(
							 pout->act_session,
							 pout->version25.name, 
							 STlength(pout->version25.name), 
							 temp_buffer->block, 
							 temp_buffer->position - 1,
							 pout->version25.level);
			CLEAR_STATUS( WPSFreeBuffer(temp_buffer) );
		    }
		    MEfree((PTR) temp_buffer);
		}
		pout->version25.repeat = FALSE;
	    }
	    else
	    {
		pout->error = WSMAddVariable(
					     pout->act_session,
					     pout->version25.name, 
					     STlength(pout->version25.name), 
					     pout->version25.value, 
					     (pout->version25.value == NULL) ? \
						0 : \
						STlength(pout->version25.value),
					     pout->version25.level);
	    }
	}	
    }
    return(GSTAT_OK); 
}

/*
** Name: WPSHtmlServer() - Execute server or user function
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
**      20-Apr-1999 (fanra01)
**          Correct trace of user_error.
**      10-Oct-2000 (fanra01)
**          Set the nb_of_columns according to parameter size.
*/
GSTATUS 
WPSHtmlServer(
    PPARSER_IN  in,
    PPARSER_OUT out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        PSERVER_FUNCTION    func;
        in->position = in->current + 1;

        if (pout->version25.name != NULL && pout->version25.name[0] != EOS)
        {
            WPS_PHTML_OVER_TEXT over;
            WPS_PHTML_TAG_TEXT tag;
            u_i4  numberOfCols;
            char *data = NULL;
            u_i4 i;
            char *dll_name = pout->version25.value;
            char *fct_name = pout->version25.name;
            char *user_error;

            pout->error = WSFGetServerFunction(dll_name, fct_name, &func);
            if (pout->error == GSTAT_OK)
            {
                numberOfCols = func->params_size;
                /*
                ** For a function set the number of columns according to
                ** the function parameter number.  Otherwise it is possible
                ** that the column number is taken from a previous query.
                */
                pout->version20.query.nb_of_columns = numberOfCols;

                pout->error = G_ME_REQ_MEM( 0, pout->format.tagsColTab,
                    char*, numberOfCols );
                if (pout->error == GSTAT_OK)
                {
                    for (i = 0; i < numberOfCols; i++)
                    {
                        data = func->params[i];
                        for (over = pout->headers;
                             pout->error == GSTAT_OK && over != NULL;
                            over = over->next)
                        {
                            if (over->name != NULL &&
                                STcompare(data, over->name) == 0)
                            {
                                pout->format.tagsColTab[i] = over->data;
                                over->data = NULL;
                                break;
                            }
                        }
                        for (tag = pout->format.firstTag; tag != NULL;
                            tag = tag->next)
                        {
                            if (tag->type == HTML_ICE_HTML_VAR &&
                                tag->name != NULL &&
                                STcompare(data, tag->name + 1) == 0)
                                tag->position = i + 1;
                        }
                    }
                }
                tag = pout->format.firstTag;
                while (tag != NULL)
                {
                    /*
                    ** check unknown variable name.
                    ** In this way the variable becomes a text
                    */
                    if (tag->position == 0)
                        tag->type = HTML_ICE_HTML_TEXT;
                    tag = tag->next;
                }

                if (pout->error == GSTAT_OK)
                {
                    PTR user_info = NULL;
                    u_i4 count = 0;
                    bool print = FALSE;
                    typedef GSTATUS    ( *SVRFCT )( ACT_PSESSION, char**, bool*, PTR*);
                    typedef char*    ( *USRFCT )( char**, bool*, PTR*);

                    SVRFCT svrfct;
                    USRFCT usrfct;

                    if (dll_name == NULL)
                        svrfct = (SVRFCT) func->fct;
                    else
                        usrfct = (USRFCT) func->fct;

                    do
                    {
                        if (dll_name == NULL)
                        {
                            pout->error = svrfct(
                                pout->act_session,
                                pout->format.tagsColTab,
                                &print,
                                &user_info);
                        }
                        else
                        {
                            asct_trace( ASCT_EXEC )
                                ( ASCT_TRACE_PARAMS,
                                "Invoking User Function=%s", func->name);

                            user_error = usrfct(
                                pout->format.tagsColTab,
                                &print,
                                &user_info);

                            if (user_error != NULL)
                            {
                                asct_trace( ASCT_EXEC )
                                    ( ASCT_TRACE_PARAMS,
                                    "User Function=%s status=%s", func->name,
                                    user_error);

                                pout->error = DDFStatusAlloc( E_WS0063_WSF_USER_FCT_ERR );
                                DDFStatusInfo(pout->error, user_error);
                            }
                            else
                            {
                                asct_trace( ASCT_EXEC )
                                    ( ASCT_TRACE_PARAMS,
                                    "User Function=%s status=%08x", func->name, 0);
                            }
                        }

                        if (pout->error == GSTAT_OK && print == TRUE)
                        {
                            count++;
                            pout->error = formatterTab[HTML_ICE_TYPE_RAW].RowEnd (
                                &pout->format,
                                &pout->buffer,
                                FALSE);
                        }
                    } while (pout->error == GSTAT_OK && user_info != NULL);

                    if (pout->error == GSTAT_OK)
                    {
                        char tmp[NUM_MAX_INT_STR];
                        CVna(count, tmp);
                        pout->error = WSMAddVariable(
                            pout->act_session,
                            HVAR_ROWCOUNT,
                            STlength(HVAR_ROWCOUNT),
                            tmp,
                            STlength(tmp),
                            WSM_ACTIVE );
                    }
                }
            }
        }
    }
    return(GSTAT_OK);
}

/*
** Name: WPSHtmlRepeat() - Manage the repeat tag
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlRepeat(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS					err = GSTAT_OK;	
	WPS_HTML_PPARSER_OUT	pout;

	pout = (WPS_HTML_PPARSER_OUT) out;
	pout->version25.repeat = TRUE;
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlEnd() - End of the macro command
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlEnd(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT	pout;
	WPS_PHTML_OVER_TEXT		over;
	WPS_PHTML_TAG_TEXT		tag;
	WPS_PHTML_QUERY_TEXT	stmt;
	WPS_PHTML_IF			tmpif;

	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK &&
		pout->buffer.position > 0)
	{
		if (pout->version25.repeat == TRUE)
		{
			WPS_PBUFFER		temp_buffer;
			pout->error = G_ME_REQ_MEM(0, temp_buffer, WPS_BUFFER, 1);
			if (pout->error == GSTAT_OK)
				pout->error	= WPSBlockInitialize(temp_buffer);

			if (pout->error == GSTAT_OK)
			{
				WPS_HTML_PPARSER_OUT out;
				PARSER_IN in;

				MEfill (sizeof (PARSER_IN), 0, (PTR)&in);
                                in.first_node = HTML_FIRST_NODE;
                                in.buffer = NULL;
                                in.size = pout->buffer.size;

				pout->error = G_ME_REQ_MEM(0, out, WPS_HTML_PARSER_OUT, 1);
				if (pout->error == GSTAT_OK) 
				{
					u_i4 read = 0;
					i4 total;
					out->act_session = pout->act_session;
					out->result = temp_buffer;
					out->active_error = pout->active_error;

					pout->error	= WPSBlockInitialize(&out->buffer);

					if (pout->error == GSTAT_OK)
						pout->error = ParseOpen(html, &in, (PPARSER_OUT) out);

					if (pout->error == GSTAT_OK)
						do 
						{
							in.length -= in.position;
							in.current = in.length;
							in.position = 0;
							pout->error = WPSBufferGet(&pout->buffer, 
																				 &read, 
																				 &total, 
																				 &in.buffer, 
																				 in.length);
							in.length += read;
							if (pout->error == GSTAT_OK && read > 0)
								pout->error = Parse(html, &in, (PPARSER_OUT) out);
						}
						while (pout->error == GSTAT_OK && read > 0);

					if (pout->error == GSTAT_OK)
						pout->error = ParseClose(html, &in, (PPARSER_OUT) out);

					pout->active_error = out->active_error;

					CLEAR_STATUS( WPSFreeBuffer(&out->buffer) );
					MEfree((PTR)out);
				}

				CLEAR_STATUS( WPSMoveBlock(pout->result, temp_buffer) );
				CLEAR_STATUS( WPSFreeBuffer(temp_buffer) );
				MEfree((PTR) temp_buffer);

				WPS_BUFFER_EMPTY_BLOCK(&pout->buffer);
			}
		}
		else
			pout->error = WPSMoveBlock(pout->result, &pout->buffer);
	}
	in->position = in->current + 1;
	
	if (pout->error != GSTAT_OK)
	{
		pout->error = WPSSetError(pout, ERROR_LVL_ERROR);
		in->current_node = HTML_FIRST_NODE;
	}
	else
		pout->error = WPSSetSuccess(pout);

	MEfree((PTR)pout->format.linksColTab);
	pout->format.linksColTab = NULL;
	while (pout->version20.links != NULL) 
	{
		over = pout->version20.links;
		pout->version20.links = pout->version20.links->next;
		MEfree((PTR)over->name);
		MEfree((PTR)over->data);
		MEfree((PTR)over);
	}

	MEfree((PTR)pout->format.headersColTab);
	pout->format.headersColTab = NULL;
	while (pout->headers != NULL) 
	{
		over = pout->headers;
		pout->headers = pout->headers->next;
		MEfree((PTR)over->name);
		MEfree((PTR)over->data);
		MEfree((PTR)over);
	}

	if (pout->format.tagsColTab != NULL)
	{
		u_i4 i;
		for (i = 0; i < pout->version20.query.nb_of_columns; i++)
			MEfree(pout->format.tagsColTab[i]);
		MEfree((PTR)pout->format.tagsColTab);
		pout->format.tagsColTab = NULL;
	}

	while (pout->format.firstTag != NULL) 
	{
		tag = pout->format.firstTag;
		pout->format.firstTag = pout->format.firstTag->next;
		MEfree((PTR)tag->name);
		MEfree((PTR)tag);
	}
	pout->format.lastTag = NULL;

	while (pout->version20.query.firstSql != NULL) 
	{
		stmt = pout->version20.query.firstSql;
		pout->version20.query.firstSql = pout->version20.query.firstSql->next;
		MEfree((PTR)stmt->stmt);
		MEfree((PTR)stmt);
	}

	while (pout->if_info != NULL)
	{
		tmpif = pout->if_info;
		pout->if_info = pout->if_info->next;
		MEfree((PTR)tmpif->text);
		MEfree((PTR)tmpif);
	}

	MEfree((PTR)pout->version20.query.dbname);
	pout->version20.query.dbname = NULL;
	MEfree((PTR)pout->version20.query.user);
	pout->version20.query.user = NULL;
	MEfree((PTR)pout->version20.query.password);
	pout->version20.query.password = NULL;
	MEfree((PTR)pout->version20.query.nullvar);
	pout->version20.query.nullvar = NULL;
	pout->version20.query.lastSql = NULL;
	MEfree((PTR)pout->format.attr);
	pout->format.attr = NULL;
	MEfree((PTR)pout->version25.name);
	pout->version25.name = NULL;
	MEfree((PTR)pout->version25.value);
	pout->version25.value = NULL;
	MEfree((PTR)pout->version25.transaction);
	pout->version25.transaction = NULL;
	MEfree((PTR)pout->version25.cursor);
	pout->version25.cursor = NULL;
return(GSTAT_OK); 
}

/*
** Name: WPSAddIfItem() - Add item into the conditional list
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	WPS_HTML_PPARSER_OUT pout 
**	u_i4				 type
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
WPSAddIfItem(
	WPS_HTML_PPARSER_OUT pout, 
	u_i4				 type)
{
	GSTATUS	err = GSTAT_OK;
	WPS_PHTML_IF tmp;
	err = G_ME_REQ_MEM(0, tmp, WPS_HTML_IF, 1);
	if (err == GSTAT_OK)
	{
		tmp->type = type;
		tmp->next = pout->if_info;
		pout->if_info = tmp;
	}
return(err); 
}

/*
** Name: WPSHtmlIfOpen() - open conditional parenthesis
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfOpen(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_OPEN_PARENTH);
		pout->if_par_counter++;
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfClose() - close conditional parenthesis
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfClose(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_CLOSE_PARENTH);
		pout->if_par_counter--;
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfLogic() - compute the conditional value
**
** Description:
**
** Inputs:
**	WPS_PHTML_IF *	: condition pile
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
**      20-Oct-98 (fanra01)
**          Change result type to signed for comparisons.
**      23-Jan-2001 (somsa01)
**          Added Null check on left->text and right->text because
**          STcompare() will SEGV ir either pointer is null when it
**          is dfined to be strcmp().
*/
GSTATUS 
WPSHtmlIfLogic(
	WPS_PHTML_IF *pile)
{
	WPS_PHTML_IF left, op, right;
	i4 result;

	if ((*pile) != NULL)
	{
		switch ((*pile)->type) 
		{
		case WPS_CLOSE_PARENTH:
			op = (*pile)->next;
			WPSHtmlIfLogic(&op);
			left = *pile;
			*pile = op;
			MEfree((PTR) left);

			WPSHtmlIfLogic(pile);
			break;

		case WPS_OPEN_PARENTH:
			op = *pile;
			*pile = (*pile)->next;
			MEfree((PTR) op);
			break;

		case WPS_TEXT:
			right = (*pile);
			op = (*pile)->next;
			left = (*pile)->next->next;

			result = STcompare(
				    (left->text == NULL) ? "" : left->text,
                        	    (right->text == NULL) ? "" : right->text);

			if ((op->type == WPS_EQUAL && result == 0) ||
				(op->type == WPS_NOT_EQUAL && result != 0) ||
				(op->type == WPS_LOWER && result < 0) ||
				(op->type == WPS_GREATER && result > 0))
				op->type = WPS_TRUE;
			else
				op->type = WPS_FALSE;

			op->next = left->next;
			*pile = op;

			MEfree((PTR)left->text);
			MEfree((PTR)left);
			MEfree((PTR)right->text);
			MEfree((PTR)right);

			WPSHtmlIfLogic(pile);
			break;

		case WPS_TRUE:
		case WPS_FALSE:
			right = (*pile);
			op = (*pile)->next;
			if (op != NULL &&
				(op->type == WPS_AND ||
				op->type == WPS_OR))
			{
				left = (*pile)->next->next;
				WPSHtmlIfLogic(&left);

				if ((op->type == WPS_AND &&
					 right->type == WPS_TRUE && 
					 left->type == WPS_TRUE) ||
					(op->type == WPS_OR &&
					 (right->type == WPS_TRUE || 
					  left->type == WPS_TRUE)))
					left->type = WPS_TRUE;
				else
					left->type = WPS_FALSE;

				*pile = left;
				MEfree((PTR)right);
				MEfree((PTR)op);
			} 
			else
			{
				WPSHtmlIfLogic(&op);
				right->next = op;
				*pile = right;
			}
			break;
		default :;
		}
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfCond() - compute the condition
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfCond(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		if (pout->if_info == NULL)
			pout->error = DDFStatusAlloc(E_WS0071_WPS_BAD_CONDITION);
		if (pout->if_par_counter != 0)
			pout->error = DDFStatusAlloc(E_WS0070_WPS_MISSING_PARENTH);
		else
			pout->error = WPSHtmlIfLogic(&pout->if_info);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfItem() - find conditional item
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfItem(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		if (pout->if_info == NULL ||
			pout->if_info->type != WPS_TEXT)
			pout->error = WPSAddIfItem(pout, WPS_TEXT);

		if (pout->error == GSTAT_OK)
			pout->error = WPSHtmlSetInfo(&pout->if_info->text, pout, in);

		in->position = in->current;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlIfGre() - find >
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfGre(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_GREATER);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfGre() - find <
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfLow(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_LOWER);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfNEq() - find !=
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfNEq(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_NOT_EQUAL);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfEq() - find ==
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfEq(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_EQUAL);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfDef() - find conditional item
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfDef(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		u_i4 length;
		char *value = NULL;
		char *name;

		name = in->buffer + in->position;
		length = in->current - in->position;
		
		pout->error = WSMGetVariable(
						pout->act_session, 
						name, 
						length, 
						&value, 
						WSM_ACTIVE | WSM_USER | WSM_SYSTEM);
		if (pout->error == GSTAT_OK)
			if (value == NULL)
				pout->error = WPSAddIfItem(pout, WPS_FALSE);
			else
				pout->error = WPSAddIfItem(pout, WPS_TRUE);
		in->position = in->current + 1;
	}
return(GSTAT_OK);
}

/*
** Name: WPSHtmlIfAnd() - find AND
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfAnd(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_AND);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfOr() - find OR
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfOr(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSAddIfItem(pout, WPS_OR);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfThen() - Add text or variable if the condition is true
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfThen(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK &&
		pout->if_info->type == WPS_TRUE)
	{
		pout->error = HtmlAppend(in, pout);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlIfElse() - Add text or variable if the condition is false
**
** Description:
**	clean the out parser structure
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlIfElse(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK &&
			pout->if_info->type == WPS_FALSE)
	{
		pout->error = HtmlAppend(in, pout);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlTransaction() - Initialize transaction name for 2.5 features
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlTransaction(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version25.transaction, pout, in);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlCursor() - Initialize cursor name for 2.5 features
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlCursor(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version25.cursor, pout, in);
		pout->version25.rows = 1;
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlRows() - Initialize number of rows for 2.5 features
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlRows(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		char *tmp = NULL;
		pout->error = WPSHtmlSetInfo(&tmp, pout, in);
		CVan(tmp, &pout->version25.rows);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlCommit() - Commit a transaction
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlCommit(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		USR_PTRANS	current = NULL;
		pout->error = WSMGetTransaction(
							pout->act_session->user_session, 
							pout->version25.transaction, 
							&current);

		if (pout->error == GSTAT_OK) 
			if (current == NULL)
				pout->error = DDFStatusAlloc( E_WS0074_WSM_BAD_TRANSACTION );
			else
				pout->error = WSMCommit(current, FALSE);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlRollback() - Rollback a transaction
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlRollback(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		USR_PTRANS	current = NULL;
		pout->error = WSMGetTransaction(
							pout->act_session->user_session, 
							pout->version25.transaction, 
							&current);

		if (pout->error == GSTAT_OK) 
			if (current == NULL)
				pout->error = DDFStatusAlloc( E_WS0074_WSM_BAD_TRANSACTION );
			else
				pout->error = WSMRollback(current, FALSE);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlDoubleChar() - Convert double character to one
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlDoubleChar(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSBlockAppend(&pout->buffer, in->buffer + in->position, 1);
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSWSrc() 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlSWSrc(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		pout->error = WPSHtmlSetInfo(&pout->version25.name, pout, in);
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSWNext() - 
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlSWNext(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		MEfree((PTR)pout->version25.value);
		pout->version25.value = NULL;
		in->position = in->current + 1;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSWCase() - 
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlSWCase(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		if (pout->version25.name != NULL && 
				pout->version25.value != NULL &&
				STcompare(pout->version25.name, pout->version25.value) == 0)
		{
			pout->error = HtmlAppend(in, pout);
			pout->version25.rows = 1;
		}
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlSWDefault() - 
**
** Description:
**	
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
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
WPSHtmlSWDefault(
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	WPS_HTML_PPARSER_OUT pout;
	pout = (WPS_HTML_PPARSER_OUT) out;
	if (pout->error == GSTAT_OK)
	{
		if (pout->version25.rows == -1)
		{
			pout->error = HtmlAppend(in, pout);
		}
		in->position = in->current;
	}
return(GSTAT_OK); 
}

/*
** Name: WPSHtmlLibName() - return name of library
**
** Description:
**      Retrieve the name of the library for the function call.
**      The position is incremented to skip the '.' delimiter.
**
** Inputs:
**      PPARSER_IN  in
**      PPARSER_OUT out
**
** Outputs:
**      None.
**
** Returns:
**      GSTATUS:    GSTAT_OK
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      28-Oct-98 (fanra01)
**          Created.
*/
GSTATUS
WPSHtmlLibName(
    PPARSER_IN  in,
    PPARSER_OUT out)
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    if (pout->error == GSTAT_OK)
    {
        pout->error = WPSHtmlSetInfo(&pout->version25.value, pout, in);
        in->position = in->current + 1;
    }
    return(GSTAT_OK);
}

/*
** Name: WPSXmlTypeRaw
**
** Description:
**      Sets the output option type to XML_ICE_TYPE_RAW.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**      None.
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      11-Oct-2000 (fanra01)
**          Created.
*/
GSTATUS 
WPSXmlTypeRaw( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    pout->version20.query.type = XML_ICE_TYPE_RAW;
    return(GSTAT_OK);
}

/*
** Name: WPSXmlTypePdata
**
** Description:
**      Sets the output option type to XML_ICE_TYPE_PDATA.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**      None.
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      11-Oct-2000 (fanra01)
**          Created.
*/
GSTATUS 
WPSXmlTypePdata( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    pout->version20.query.type = XML_ICE_TYPE_PDATA;
    return(GSTAT_OK);
}

/*
** Name: WPSXmlTypeXml
**
** Description:
**      Sets the output option type to XML_ICE_TYPE_XML.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**      None.
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXmlTypeXml( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    pout->version20.query.type = XML_ICE_TYPE_XML;
    return(GSTAT_OK);
}

/*
** Name: WPSXmlTypeXmlPdata
**
** Description:
**      Sets the output option type to XML_ICE_TYPE_XML_PDATA.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**      None.
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXmlTypeXmlPdata( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout;
    pout = (WPS_HTML_PPARSER_OUT) out;
    pout->version20.query.type = XML_ICE_TYPE_XML_PDATA;
    return(GSTAT_OK);
}

/*
** Name: WPSMarkupText
**
** Description:
**      Sets the text within the XML macro tag.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      11-Oct-2000 (fanra01)
**          Created based on WPSHtmlHtmlText.
*/
GSTATUS 
WPSMarkupText( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout = (WPS_HTML_PPARSER_OUT) out;

    if (pout->error == GSTAT_OK)
    {
        WPS_PHTML_TAG_TEXT tag = NULL;

        if (in->current > in->position)
        {
            pout->error = G_ME_REQ_MEM(0, tag, WPS_HTML_TAG_TEXT, 1);
            if (pout->error == GSTAT_OK)
            {
                pout->error = WPSHtmlSetInfo(&tag->name, pout, in);
                if (pout->error == GSTAT_OK)
                {
                    tag->type = HTML_ICE_HTML_TEXT;
                    if (pout->format.firstTag == NULL)
                        pout->format.firstTag = pout->format.lastTag = tag;
                    else
                        pout->format.lastTag = pout->format.lastTag->next = tag;
                }
            }
        }
        in->position = in->current;
        if (in->buffer[in->position] == '`')
            in->position++;
    }
    return(GSTAT_OK);
}

/*
** Name: WPSXmlMarkupVar
**
** Description:
**      Update the output tag lists an set the column tag types to
**      HTML_ICE_HTML_VAR.
**
** Inputs:
**	in          parser input structure
**	out         parser result structure
**
** Outputs:
**
** Returns:
**	GSTAT_OK    command completed successfully
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      11-Oct-2000 (fanra01)
**          Created based on WPSHtmlHtmlVar.
*/
GSTATUS 
WPSXmlMarkupVar( PPARSER_IN in, PPARSER_OUT out )
{
    WPS_HTML_PPARSER_OUT pout = (WPS_HTML_PPARSER_OUT) out;

    if (pout->error == GSTAT_OK)
    {
        WPS_PHTML_TAG_TEXT tag;

        pout->error = G_ME_REQ_MEM(0, tag, WPS_HTML_TAG_TEXT, 1);
        if (pout->error == GSTAT_OK)
        {
            pout->error = WPSHtmlSetInfo(&tag->name, pout, in);
            if (pout->error == GSTAT_OK)
            {
                tag->type = HTML_ICE_HTML_VAR;
                if (pout->format.firstTag == NULL)
                    pout->format.firstTag = pout->format.lastTag = tag;
                else
                    pout->format.lastTag = pout->format.lastTag->next = tag;
            }
        }
        in->position = in->current;
        if (in->buffer[in->position] == '`')
            in->position++;
    }
    return(GSTAT_OK);
}
