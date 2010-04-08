/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <htmlform.h>
#include <wsf.h>

/*
**
**  Name: htmlform.c - Html Format
**
**  Description:
**		Generate HTML format from information returned by a database query
**		The format depends of the TYPE tag defined in the macro
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      15-Apr-1999 (fanra01)
**          When selecting blobs with the HTML option the data was misplaced
**          on the page.  Add format tag checking if they exist.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Oct-2000 (fanra01)
**          Sir 103096
**          Add experimental formatting functions to handle XML output.
**          WPSXMLRawItem deals with data output.
**          XMLEscapeEntity replaces entity literals with the escaped form.
**      22-Jun-2001 (fanra01)
**          Sir 103096
**          Add XML formatting routines for processing XML and add references
**          to the action table.
**	17-aug-2001 (somsa01)
**	    Corrected "if" statement in XMLEscapeEntity().
*/

/*
** Name: URLEscapeChar() 
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	 : buffer
**	char		 : val
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
URLEscapeChar (
	WPS_PBUFFER	 buffer,
	char val) 
{
	GSTATUS err = GSTAT_OK;
	char tmp[10];

  if (IsSeparator (val) == TRUE)
		err = WPSBlockAppend(buffer, ERx("+"), 1);
  else 
	{
		if (IsAlpha (val) == TRUE || IsDigit (val) == TRUE)
			STprintf (tmp, "%c", val);
	  else 
			STprintf (tmp, "%%%2.2x", (u_i4)val);
		err = WPSBlockAppend(buffer, tmp, STlength(tmp));
	}
return(err);
}

/*
** Name: AppendLink() - Type LINK
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: value
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
AppendLink (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* value) 
{
	GSTATUS err = GSTAT_OK;
	
    if (query->linksColTab != NULL && 
		query->linksColTab[col-1] != NULL && 
		query->linksColTab[col-1]->data != NULL) 
	{
		err = WPSBlockAppend(buffer, TAG_LINK, STlength (TAG_LINK));
		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					query->linksColTab[col-1]->data, 
					STlength (query->linksColTab[col-1]->data));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					STR_QUESTION_MARK, 
					STlength (STR_QUESTION_MARK));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					query->linksColTab[col-1]->name, 
					STlength (query->linksColTab[col-1]->name));

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					STR_EQUALS_SIGN, 
					STlength (STR_EQUALS_SIGN));

		while (err == GSTAT_OK && value[0] != EOS)
            err = URLEscapeChar (buffer, *value++);

		if (err == GSTAT_OK)
			err = WPSBlockAppend(
					buffer, 
					TAG_END_LINK_URL, 
					STlength (TAG_END_LINK_URL));
    }
return (err);
}

/*
** Name: AppendEndLink() - Type LINK
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
AppendEndLink (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col) 
{
	GSTATUS err = GSTAT_OK;
	
    if (query->linksColTab != NULL && 
		query->linksColTab[col-1] != NULL && 
		query->linksColTab[col-1]->data != NULL) 
	{
		err = WPSBlockAppend(
				buffer, 
				TAG_END_LINK, 
				STlength (TAG_END_LINK));
    }
return (err);
}

/*
** Name: WPSHTMLSelectBegin() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectBegin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;

	err = WPSBlockAppend(buffer, TAG_SELECT, STlength (TAG_SELECT));

	if (err == GSTAT_OK && query->attr != NULL)
        err = WPSBlockAppend(buffer, query->attr, STlength (query->attr));

	if (err == GSTAT_OK)
		err = WPSBlockAppend(buffer, END_TAG, STlength(END_TAG));
	
	if (err == GSTAT_OK)
		err = WPSBlockAppend(buffer, STR_CRLF, STlength(STR_CRLF));

return (err);
}

/*
** Name: WPSHTMLSelectEnd() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectEnd (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	err = WPSBlockAppend(buffer, TAG_END_SELECT, STlength(TAG_END_SELECT));
return(err);
}

/*
** Name: WPSHTMLSelectRowBegin() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectRowBegin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
    if (fHeader == FALSE)
		err = WPSBlockAppend(buffer, TAG_OPTION, STlength(TAG_OPTION));
return(err);
}

/*
** Name: WPSHTMLSelectRowEnd() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectRowEnd (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
	if (fHeader == FALSE)
		err = WPSBlockAppend(buffer, TAG_END_OPTION, STlength(TAG_END_OPTION));
return(err);
}

/*
** Name: WPSHTMLSelectItem() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectItem (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* data, 
	bool fHeader) 
{
    GSTATUS err = GSTAT_OK;

	if (fHeader == FALSE) 
	{
		err = AppendLink (query, buffer, col, data);
		if (err == GSTAT_OK && data != NULL)
			err = WPSBlockAppend(buffer, data, STlength(data));
		if (err == GSTAT_OK)
			err = AppendEndLink (query, buffer, col);
	}
return(err);
}

/*
** Name: WPSHTMLSelectItemBin() - Type SELECT
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLSelectItemBin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* data, 
	bool fHeader) 
{
    GSTATUS err = GSTAT_OK;
	if (fHeader == FALSE)
		err = WPSBlockAppend(buffer, STR_WPS_BINARY, STlength(STR_WPS_BINARY));
return(err);
}

/*
** Name: WPSHTMLRawItem() - Type RAW
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLRawItem (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* data, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
	if (fHeader == TRUE)
		return(err);
	
	if (query->firstTag == NULL)
	{
		if (data != NULL && *data != EOS)
		{
			u_i4 len;
			len = STlength(data);
			err = WPSBlockAppend(buffer, data, len);
		}
	}
	else
	{
		if (data == NULL)
		{
			MEfree(query->tagsColTab[col-1]);
			query->tagsColTab[col-1] = NULL;
		} 
		else
		{
			u_i4 length = STlength(data) + 1;
			err = GAlloc((PTR*)(PTR*) &(query->tagsColTab[col-1]), length, FALSE);
			if (err == GSTAT_OK)
				MECOPY_VAR_MACRO(data, length, query->tagsColTab[col-1]);
		}
	}
return (err);
}

/*
** Name: WPSHTMLRawItemBin() - Type RAW
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
**          Add format tag checking if there are any.
*/
GSTATUS 
WPSHTMLRawItemBin (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;

    if (fHeader == TRUE)
        return(err);

    if (query->firstTag == NULL)
    {
        if (data != NULL && *data != EOS)
        {
            err = WPSBlockAppend(buffer, data, STlength (data));
        }
    }
    else
    {
        if (data == NULL)
        {
            MEfree(query->tagsColTab[col-1]);
            query->tagsColTab[col-1] = NULL;
        }
        else
        {
            u_i4 length = STlength(data) + 1;
            err = GAlloc((PTR*) &(query->tagsColTab[col-1]), length, FALSE);
            if (err == GSTAT_OK)
                MECOPY_VAR_MACRO(data, length, query->tagsColTab[col-1]);
        }
    }

    return (err);
}

/*
** Name: WPSHTMLPlainRowBegin() - Type IMAGE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLPlainRowBegin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
    if (fHeader == FALSE)
		err = WPSBlockAppend(buffer, TAG_PARAGRAPH, STlength(TAG_PARAGRAPH));
return (err);
}

/*
** Name: WPSHTMLImgSrcItem() - Type IMAGE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLImgSrcItem (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;

    if (err == GSTAT_OK && fHeader == FALSE && data != NULL && *data != EOS)
    {
        if (query->linksColTab != NULL &&
            query->linksColTab[col-1] != NULL &&
            query->linksColTab[col-1]->data != NULL)
        {
            err = WPSBlockAppend(
                    buffer,
                    TAG_LINK,
                    STlength(TAG_LINK));

            if (err == GSTAT_OK)
                err = WPSBlockAppend(
                        buffer,
                        query->linksColTab[col-1]->data,
                        STlength(query->linksColTab[col-1]->data));

            if (err == GSTAT_OK)
                err = WPSBlockAppend(
                        buffer,
                        TAG_END_LINK_URL,
                        STlength(TAG_END_LINK_URL));
        }

        if (err == GSTAT_OK)
        {
            err = WPSBlockAppend(buffer, TAG_IMGSRC, STlength (TAG_IMGSRC));
            err = WPSHTMLRawItemBin(query, buffer, col, data, fHeader);
            err = WPSBlockAppend(buffer, STR_QUOTE, STlength (STR_QUOTE));
            if (query->attr != NULL && *query->attr != EOS)
                err = WPSBlockAppend(buffer, query->attr, STlength (query->attr));
            err = WPSBlockAppend(buffer, END_TAG, STlength (END_TAG));
        }

        if (err == GSTAT_OK &&
            fHeader == FALSE &&
            query->linksColTab != NULL &&
            query->linksColTab[col-1] != NULL &&
            query->linksColTab[col-1]->data != NULL)
            err = WPSBlockAppend(buffer, TAG_END_LINK, STlength(TAG_END_LINK));
    }
    return (err);
}

/*
** Name: WPSHTMLTableBegin() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableBegin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
    err = WPSBlockAppend(buffer, TAG_TABLE, STlength(TAG_TABLE));
    if (query->attr != NULL)
		err = WPSBlockAppend(buffer, query->attr, STlength(query->attr));
    else
    {
		err = WPSBlockAppend(buffer, ATTR_BORDER, STlength(ATTR_BORDER));
		if (err == GSTAT_OK)
			err = WPSBlockAppend(buffer, ATTR_DEF_CELLSPACING, STlength(ATTR_DEF_CELLSPACING));
    }
	if (err == GSTAT_OK)
		err = WPSBlockAppend(buffer, END_TAG, STlength(END_TAG));
return (err);
}

/*
** Name: WPSHTMLTableEnd() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableEnd (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	err = WPSBlockAppend(buffer, TAG_END_TABLE, STlength(TAG_END_TABLE));
return (err);
}

/*
** Name: WPSHTMLTableRowBegin() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableRowBegin (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
	err = WPSBlockAppend(buffer, TAG_TABROW, STlength(TAG_TABROW));
return (err);
}

/*
** Name: WPSHTMLTableRowEnd() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableRowEnd (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
	err = WPSBlockAppend(buffer, TAG_END_TABROW, STlength(TAG_END_TABROW));
return (err);
}

/*
** Name: WPSHTMLTableCell() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableCell (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* data, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK; 

  if (fHeader == TRUE) 
	{
		err = WPSBlockAppend(buffer, TAG_TABHEAD, STlength(TAG_TABHEAD));
		if (err == GSTAT_OK)
		  if (query->headersColTab != NULL && 
				query->headersColTab[col-1] != NULL) 
				err = WPSBlockAppend(
						buffer, 
						query->headersColTab[col-1]->data, 
						STlength(query->headersColTab[col-1]->data));
			else if (data != NULL)
				err = WPSBlockAppend(buffer, data, STlength(data));
		if (err == GSTAT_OK)
			err = WPSBlockAppend(buffer, TAG_END_TABHEAD, STlength(TAG_END_TABHEAD));
	}
    else 
	{
		err = WPSBlockAppend(buffer, TAG_TABCELL, STlength(TAG_TABCELL));
		if (err == GSTAT_OK)
			err = AppendLink (query, buffer, col, data);
		if (err == GSTAT_OK && data != NULL)
			err = WPSBlockAppend(buffer, data, STlength(data));
	  if (err == GSTAT_OK)
			err = AppendEndLink (query, buffer, col);
		if (err == GSTAT_OK)
			err = WPSBlockAppend(buffer, TAG_END_TABCELL, STlength(TAG_END_TABCELL));
    }
return (err);
}

/*
** Name: WPSHTMLTableCellImage() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLTableCellImage (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	u_i4 col, 
	char* data, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
    
    if (fHeader == TRUE)
        err = WPSBlockAppend(buffer, TAG_TABHEAD, STlength(TAG_TABHEAD));
    else {
        err = WPSBlockAppend(buffer, TAG_TABCELL, STlength(TAG_TABCELL));
		MEfree((PTR)query->attr);
		err = WPSHTMLImgSrcItem(query, buffer, col, data, fHeader);
    }
return(err);
}

/*
** Name: WPSHTMLRawRowEnd() - Type RAW
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
WPSHTMLRawRowEnd (
	WPS_PHTML_FORMAT_INFO query, 
	WPS_PBUFFER buffer, 
	bool fHeader) 
{
	GSTATUS err = GSTAT_OK;
	if (fHeader == FALSE)
	{
		WPS_PHTML_TAG_TEXT tag;
		tag = query->firstTag;
		while (err == GSTAT_OK && tag != NULL)
		{
			if (tag->type & HTML_ICE_HTML_TEXT &&
				tag->name != NULL)
					err = WPSBlockAppend(buffer, tag->name, STlength(tag->name));
			else if (tag->type & HTML_ICE_HTML_VAR &&
					 query->tagsColTab[tag->position-1] != NULL)
					err = WPSBlockAppend(buffer, 
							query->tagsColTab[tag->position-1], 
							STlength(query->tagsColTab[tag->position-1]));
			tag = tag->next;
		}
	}
return (err);
}

/*
** Name: XMLEscapeEntity
**
** Description:
**      Converts the XML literals within data to the escaped form.
**
** Inputs:
**      data        pointer to the output string value
**      outlen      length of output string
**
** Outputs:
**      outdata     returned converted string
**      outlen      new length of string
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
**      22-Jun-2001 (fanra01)
**          Add test for NULL data pointer.  If a column returns null values
**          don't try to convert literals.
**	17-aug-2001 (somsa01)
**	    Corrected "if" statement.
*/
GSTATUS
XMLEscapeEntity( char* data, char** outdata, i4* outlen )
{
    GSTATUS err = GSTAT_OK;
    i4      len = *outlen;
    i4      i;
    char*   p = data;
    char*   o = NULL;
    char*   ret = NULL;
    i4      lit = -1;

    static char esclit[5][7] = { "&lt;", "&gt;", "&amp;", "&apos;", "&quot;" };

    /*
    ** Allocate memory assuming max possible replacements
    */
    if ((data != NULL) && (*data != 0) && (*outlen > 0))
    {
        err = GAlloc( &ret, len * 7, FALSE);
        for(i=0, o=ret;
            (err == GSTAT_OK) && (i < len) && (p != NULL) && (*p != 0);
            i+=1, p+=sizeof(char))
        {
            switch(*p)
            {
                case '\"':
                    lit += 1;
                case '\'':
                    lit += 1;
                case '&':
                    lit += 1;
                case '>':
                    lit += 1;
                case '<':
                    lit += 1;
                    break;
                default:
                    lit = -1;
                    break;
            }
            if ( lit == -1 )
            {
                *o = *p;
                o += sizeof(char);
            }
            else
            {
                STcat( o, esclit[lit] );
                o += STlength( esclit[lit] );
            }
        }
        *outdata = ret;
        *outlen = STlength( ret );
    }
    return(err);
}

/*
** Name: WPSXMLRawItem
**
** Description:
**
** Inputs:
**	query       query control structure
**	buffer      output control structure
**	col         column number
**	data        column data
**	fHeader     TRUE if the row is the Header row
**
** Outputs:
**	buffer
**
** Returns:
**	GSTAT_OK    completed successfully.
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
WPSXMLRawItem (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;
    char*   outdata = NULL;
    i4      outlen = 0;

    if (fHeader == TRUE)
        return(err);

    /*
    ** column data is not null check for XML entities and replace
    */
    if(data != NULL)
    {
        outlen =  STlength(data);
        XMLEscapeEntity( data, &outdata, &outlen );
    }

    if (query->firstTag == NULL)
    {
        if (outdata != NULL && *outdata != EOS)
        {
            err = WPSBlockAppend( buffer, outdata, outlen);
        }
    }
    else
    {
        if (outdata == NULL)
        {
            MEfree(query->tagsColTab[col-1]);
            query->tagsColTab[col-1] = NULL;
        }
        else
        {
            outlen += 1;
            err = GAlloc((PTR*)(PTR*) &(query->tagsColTab[col-1]), outlen, FALSE);
            if (err == GSTAT_OK)
                MECOPY_VAR_MACRO(outdata, outlen, query->tagsColTab[col-1]);
        }
    }
    return (err);
}

/*
** Name: WPSXMLPdataItem
**
** Description:
**
** Inputs:
**	query       query control structure
**	buffer      output control structure
**	col         column number
**	data        column data
**	fHeader     TRUE if the row is the Header row
**
** Outputs:
**	buffer
**
** Returns:
**	GSTAT_OK    completed successfully.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Jun-2000 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLPdataItem (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;
    i4      outlen = 0;

    if (fHeader == TRUE)
        return(err);

    if(data != NULL)
    {
        outlen =  STlength(data);
    }

    if (query->firstTag == NULL)
    {
        if (data != NULL && *data != EOS)
        {
            err = WPSBlockAppend( buffer, data, outlen);
        }
    }
    else
    {
        if (data == NULL)
        {
            MEfree(query->tagsColTab[col-1]);
            query->tagsColTab[col-1] = NULL;
        }
        else
        {
            outlen += 1;
            err = GAlloc((PTR*)(PTR*) &(query->tagsColTab[col-1]), outlen, FALSE);
            if (err == GSTAT_OK)
                MECOPY_VAR_MACRO(data, outlen, query->tagsColTab[col-1]);
        }
    }
    return (err);
}

/*
** Name: WPSXMLImgSrcItem
**
** Description:
**
** Inputs:
**      query
**      buffer
**      column number
**      data
**      TRUE if the row is the Header row
**
** Outputs:
**      buffer
**
** Returns:
**      GSTATUS    :    GSTAT_OK
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      18-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLImgSrcItem (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;

    /*
    ** FIX ME - This adds handling of images and binaries as an IMG SRC tag.
    ** This is valid XML but does not make use of any of the XML facilities.
    ** For application-to-application communications the object should be
    ** Base64 encoded.
    ** Add handling for blobs
    */
    if (err == GSTAT_OK)
    {
        err = WPSBlockAppend(buffer, TAG_IMGSRC, STlength (TAG_IMGSRC));
        err = WPSHTMLRawItemBin(query, buffer, col, data, fHeader);
        if (query->attr != NULL && *query->attr != EOS)
        {
            err = WPSBlockAppend(buffer, STR_QUOTE, STlength (STR_QUOTE));
            err = WPSBlockAppend(buffer, query->attr, STlength (query->attr));
            err = WPSBlockAppend(buffer, END_TAG,
                STlength( END_TAG ) );
        }
        else
        {
            err = WPSBlockAppend(buffer, TAG_STRICT_END_IMGSRC,
                STlength( TAG_STRICT_END_IMGSRC ) );
        }
    }

    return(err);
}

/*
** Name: WPSXMLSetBegin
**
** Description:
**
** Inputs:
**    query
**    buffer
**
** Outputs:
**    buffer
**
** Returns:
**    GSTATUS    :    GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetBegin(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer )
{
    GSTATUS err = GSTAT_OK;
    err = WPSBlockAppend( buffer, ING_DTD_RESULTS_OPEN,
        STlength( ING_DTD_RESULTS_OPEN ) );
    if (query->attr != NULL)
    {
        err = WPSBlockAppend(buffer, query->attr, STlength(query->attr));
    }
    if (err == GSTAT_OK)
        err = WPSBlockAppend(buffer, END_TAG, STlength(END_TAG));
    return (err);
}

/*
** Name: WPSXMLSetEnd
**
** Description:
**
** Inputs:
**    query
**    buffer
**
** Outputs:
**    buffer
**
** Returns:
**    GSTATUS    :    GSTAT_OK
**
** Exceptions:
**    None
**
** Side Effects:
**    None
**
** History:
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetEnd(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer )
{
    GSTATUS err = GSTAT_OK;
    err = WPSBlockAppend( buffer, ING_DTD_RESULTS_END,
        STlength( ING_DTD_RESULTS_END ) );
    return (err);
}

/*
** Name: WPSXMLSetElementBegin
**
** Description:
**
** Inputs:
**	query
**	buffer
**	TRUE if the row is the Header row
**
** Outputs:
**	buffer
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
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetElementBegin(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    bool fHeader )
{
    GSTATUS err = GSTAT_OK;
    err = WPSBlockAppend( buffer, ING_DTD_ROW_BEGIN,
        STlength( ING_DTD_ROW_BEGIN ) );
    return (err);
}

/*
** Name: WPSXMLSetElementEnd
**
** Description:
**
** Inputs:
**	query
**	buffer
**	TRUE if the row is the Header row
**
** Outputs:
**	buffer
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
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetElementEnd(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    bool fHeader )
{
    GSTATUS err = GSTAT_OK;
    err = WPSBlockAppend( buffer, ING_DTD_ROW_END,
        STlength( ING_DTD_ROW_END ) );
    return (err);
}

/*
** Name: WPSXMLSetChild
**
** Description:
**
** Inputs:
**	query
**	buffer
**	column number
**	data
**	TRUE if the row is the Header row
**
** Outputs:
**	buffer
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
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetChild(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;
    char*   outdata = NULL;
    i4      outlen = 0;

    if (query->headersColTab != NULL && query->headersColTab[col-1] != NULL)
    {
        err = WPSBlockAppend(buffer, ING_DTD_COL_ATTR_OPEN,
            STlength( ING_DTD_COL_ATTR_OPEN ) );
        err = WPSBlockAppend(
            buffer,
            query->headersColTab[col-1]->data,
            STlength(query->headersColTab[col-1]->data));
        err = WPSBlockAppend( buffer, ING_DTD_COL_ATTR_CLOSE,
            STlength( ING_DTD_COL_ATTR_CLOSE ) );
    }
    else
    {
        err = WPSBlockAppend( buffer, ING_DTD_COL_BEGIN,
            STlength( ING_DTD_COL_BEGIN ) );
    }
    if (err == GSTAT_OK && data != NULL)
    {
        outlen =  STlength(data);
        XMLEscapeEntity( data, &outdata, &outlen );
        if (outdata != NULL && *outdata != EOS)
        {
            err = WPSBlockAppend( buffer, outdata, outlen);
        }
    }
    if (err == GSTAT_OK)
    {
        err = WPSBlockAppend(buffer, ING_DTD_COL_END,
            STlength( ING_DTD_COL_END ));
    }
    return (err);
}

/*
** Name: WPSXMLSetChildPdata
**
** Description:
**
** Inputs:
**	query
**	buffer
**	column number
**	data
**	TRUE if the row is the Header row
**
** Outputs:
**	buffer
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
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetChildPdata(
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;

    if (query->headersColTab != NULL && query->headersColTab[col-1] != NULL)
    {
        err = WPSBlockAppend( buffer, ING_DTD_COL_ATTR_OPEN,
            STlength( ING_DTD_COL_ATTR_OPEN ) );
        err = WPSBlockAppend(
            buffer,
            query->headersColTab[col-1]->data,
            STlength(query->headersColTab[col-1]->data));
        err = WPSBlockAppend( buffer, ING_DTD_COL_ATTR_CLOSE,
            STlength( ING_DTD_COL_ATTR_CLOSE ));
    }
    else
    {
        err = WPSBlockAppend( buffer, ING_DTD_COL_BEGIN,
            STlength( ING_DTD_COL_BEGIN ) );
    }
    if (err == GSTAT_OK && data != NULL)
    {
        err = WPSBlockAppend(buffer, data, STlength(data));
    }
    if (err == GSTAT_OK)
    {
        err = WPSBlockAppend( buffer, ING_DTD_COL_END,
            STlength( ING_DTD_COL_END ) );
    }
    return (err);
}

/*
** Name: WPSXMLSetCellImage() - Type TABLE
**
** Description:
**
** Inputs:
**	WPS_PHTML_FORMAT_INFO	: query
**	WPS_PBUFFER				: buffer
**	u_i4					: column number
**	char*					: data
**	bool					: TRUE if the row is the Header row
**
** Outputs:
**	WPS_PBUFFER	 : buffer
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
**      22-Jun-2001 (fanra01)
**          Created.
*/
GSTATUS 
WPSXMLSetCellImage (
    WPS_PHTML_FORMAT_INFO query,
    WPS_PBUFFER buffer,
    u_i4 col,
    char* data,
    bool fHeader)
{
    GSTATUS err = GSTAT_OK;

    /*
    ** FIX ME - This adds handling of images and binaries as an IMG SRC tag.
    ** This is valid XML but does not make use of any of the XML facilities.
    ** For application-to-application communications the object should be
    ** Base64 encoded.
    ** Add handling for blobs
    */
    if (err == GSTAT_OK)
    {
        err = WPSBlockAppend(buffer, TAG_IMGSRC, STlength (TAG_IMGSRC));
        err = WPSHTMLRawItemBin(query, buffer, col, data, fHeader);
        if (query->attr != NULL && *query->attr != EOS)
        {
            err = WPSBlockAppend(buffer, STR_QUOTE, STlength (STR_QUOTE));
            err = WPSBlockAppend(buffer, query->attr, STlength (query->attr));
            err = WPSBlockAppend(buffer, END_TAG,
                STlength( END_TAG ) );
        }
        else
        {
            err = WPSBlockAppend(buffer, TAG_STRICT_END_IMGSRC,
                STlength( TAG_STRICT_END_IMGSRC ) );
        }
    }

    return(err);
}

GLOBALDEF HTMLFORMATTER formatterTab[] = {

/* HTML_WPS_TYPE_LINK  */
{ NULL,                 NULL,               NULL,
  NULL,                 NULL,               NULL
},

/* HTML_WPS_TYPE_OLIST */
{ NULL,                 NULL,               NULL,
  NULL,                 NULL,               NULL
},

/* HTML_WPS_TYPE_ULIST */
{ NULL,                 NULL,               NULL,
  NULL,                 NULL,               NULL
},

/* HTML_WPS_TYPE_COMBO */
{ WPSHTMLSelectBegin,   WPSHTMLSelectEnd,   WPSHTMLSelectRowBegin,
  WPSHTMLSelectRowEnd,  WPSHTMLSelectItem,  WPSHTMLSelectItemBin    },

/* HTML_WPS_TYPE_TABLE */
{ WPSHTMLTableBegin,    WPSHTMLTableEnd,    WPSHTMLTableRowBegin,
  WPSHTMLTableRowEnd,   WPSHTMLTableCell,   WPSHTMLTableCellImage    },

/* HTML_WPS_TYPE_IMAGE */
{ NULL,                 NULL,               WPSHTMLPlainRowBegin,
  NULL,                 WPSHTMLSelectItem,  WPSHTMLImgSrcItem        },

/* HTML_WPS_TYPE_RAW   */
{ NULL,                 NULL,               NULL,
  WPSHTMLRawRowEnd,     WPSHTMLRawItem,     WPSHTMLRawItemBin        },

/* XML_ICE_TYPE_RAW    */
{ NULL,                 NULL,               NULL,
  WPSHTMLRawRowEnd,     WPSXMLRawItem,     WPSHTMLRawItemBin        },

/* XML_ICE_TYPE_PDATA  */
{ NULL,                 NULL,               NULL,
  WPSHTMLRawRowEnd,     WPSXMLPdataItem,     WPSHTMLRawItemBin        },

/* XML_ICE_TYPE_XML */
{ WPSXMLSetBegin,       WPSXMLSetEnd,   WPSXMLSetElementBegin,
  WPSXMLSetElementEnd,  WPSXMLSetChild, WPSXMLSetCellImage    },

/* XML_ICE_TYPE_XML_PDATA */
{ WPSXMLSetBegin,       WPSXMLSetEnd,   WPSXMLSetElementBegin,
  WPSXMLSetElementEnd,  WPSXMLSetChildPdata, WPSXMLSetCellImage    },
};
