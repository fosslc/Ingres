/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef HTML_FORM_INCLUDED
#define HTML_FORM_INCLUDED

#include <wpsbuffer.h>

typedef struct __WPS_HTML_QUERY_TEXT 
{
	char *stmt;
	u_i4 length;
	u_i4 size;
	struct __WPS_HTML_QUERY_TEXT *next;
} WPS_HTML_QUERY_TEXT, *WPS_PHTML_QUERY_TEXT;

typedef struct __WPS_HTML_OVER_TEXT 
{
	char *name;
	char *data;
	struct __WPS_HTML_OVER_TEXT *next;
} WPS_HTML_OVER_TEXT, *WPS_PHTML_OVER_TEXT ;

typedef struct __WPS_HTML_TAG_TEXT 
{
	u_i4 type;
	char *name;
	u_i4 position;
	struct __WPS_HTML_TAG_TEXT *next;
} WPS_HTML_TAG_TEXT, *WPS_PHTML_TAG_TEXT;

typedef struct __WPS_HTML_FORMAT_INFO
{
	WPS_PHTML_OVER_TEXT  *linksColTab;
	WPS_PHTML_OVER_TEXT  *headersColTab;
	WPS_PHTML_TAG_TEXT	 firstTag, lastTag;
	char*				 *tagsColTab;
	char				*attr;
} WPS_HTML_FORMAT_INFO, *WPS_PHTML_FORMAT_INFO;

typedef GSTATUS  ( *WPS_HTMLMETHOD1 )( WPS_PHTML_FORMAT_INFO , WPS_PBUFFER);
typedef GSTATUS  ( *WPS_HTMLMETHOD2 )( WPS_PHTML_FORMAT_INFO , WPS_PBUFFER, bool);
typedef GSTATUS  ( *WPS_HTMLMETHOD3 )( WPS_PHTML_FORMAT_INFO , WPS_PBUFFER, u_i4, char *, bool);

typedef struct _HTMLFORMATTER
{
    WPS_HTMLMETHOD1	OutputStart;
    WPS_HTMLMETHOD1	OutputEnd;
    WPS_HTMLMETHOD2	RowStart;
    WPS_HTMLMETHOD2	RowEnd;
    WPS_HTMLMETHOD3	TextItem;
    WPS_HTMLMETHOD3	BinaryItem;
} HTMLFORMATTER, *PHTMLFORMATTER;

GLOBALREF HTMLFORMATTER formatterTab[];

#endif
