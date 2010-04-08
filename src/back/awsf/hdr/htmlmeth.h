/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected case for actsession to match piccolo.
**      28-Oct-98 (fanra01)
**          Add prototype for WPSHtmlLibName.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Oct-2000 (fanra01)
**          Sir 103096
**          Add function prototypes for WPSXmlTypeRaw, WPSMarkupText and
**          WPSXmlMarkupVar for basic XML support.
**      01-Dec-2000 (fanra01)
**          Bug 102857
**          Add prototype WPSAppendCharToValue.
**      22-Jun-2001 (fanra01)
**          Sir 103096
**          Add prototypes for formatting routines for XML.
*/
#ifndef HTMLMETH_INCLUDED
#define HTMLMETH_INCLUDED

#include <wsfparser.h>
#include <htmlform.h>
#include <actsession.h>

typedef struct __WPS_HTML_QUERY 
{
	char					*dbname;
	char					*user;
	char					*password;
	char					*extension;
	char					*nullvar;
	i4						type;

	WPS_PHTML_QUERY_TEXT	firstSql;
	WPS_PHTML_QUERY_TEXT	lastSql;
	u_i4					nb_of_columns;
} WPS_HTML_QUERY, *WPS_PHTML_QUERY;

typedef struct __WPS_HTML_IF 
{
	u_i4					type;
	char					*text;
	struct __WPS_HTML_IF	*next;
} WPS_HTML_IF, *WPS_PHTML_IF;

typedef struct __WPSHTML_PARSER_OUT 
{
	GSTATUS				error;
	ACT_PSESSION	act_session;
	WPS_BUFFER		buffer;
	WPS_PBUFFER		result;
	u_i4					driver;
	bool					active_error;

	struct {
		WPS_HTML_QUERY  query;
		WPS_PHTML_OVER_TEXT links; /* use to store links attribute during the parsing */
	} version20;
	struct { /* 2.5 properties */
		u_i4			level;
		char			*name;
		char			*transaction;
		char			*cursor;
		char			*value;
		bool			repeat;
		i4			  rows;
	} version25;

	WPS_PHTML_OVER_TEXT  headers; /* use to store headers for 2.0 and params for 2.5 attribute during the parsing */
	WPS_HTML_FORMAT_INFO format;

	WPS_PHTML_IF		 if_info;
	u_i4				 if_par_counter;
} WPS_HTML_PARSER_OUT, *WPS_HTML_PPARSER_OUT;

GSTATUS
WPSConvertTag  (
	char*	source,
	u_i4	src_len, 
	char*	*destination,
	u_i4	*dest_len,
	bool	keep_new_line);

GSTATUS
WPSPlainText (
	WPS_PBUFFER	destination,
	WPS_PBUFFER	source);

GSTATUS WPSHtmlInitializeMacro (
	WPS_HTML_PPARSER_OUT	pout,
	PPARSER_IN		in);

GSTATUS WPSHtmlTerminateMacro (
	WPS_HTML_PPARSER_OUT	pout,
	PPARSER_IN		in, 
	GSTATUS *status);

GSTATUS 
WPSHtmlICE(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlCopy(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlMoveTo(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlMoveAfter(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlSql(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlSqlParam(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlNewSqlQuery(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlDatabase(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlAttr(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmllinksName(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmllinksData(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlheadersName(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlheadersData(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlHtmlText(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlHtmlVar(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeLink(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeOList(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeUList(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeCombo(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeTable(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeImage(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlTypeRaw(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS 
WPSHtmlExecute(
	PPARSER_IN in, 
	PPARSER_OUT out);

GSTATUS WPSHtmlFrameDetect(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlUser(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlPassword(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlExtension(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlNullVar(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlVar (
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlName25(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlValue25(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSAppendCharToValue(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIncHTML(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIncReport(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIncExe(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIncMulti(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlInclude(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlDeclLevel(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlDecl(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlServer(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlRepeat(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfOpen(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfClose(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfCond(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfItem(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfGre(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfLow(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfNEq(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfEq(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfAnd(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfOr(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfDef(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfThen(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlIfElse(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlEnd(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlCommit(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlRollback(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlTransaction(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlCursor(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS WPSHtmlRows(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlDoubleChar(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlSWSrc(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlSWNext(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlSWCase(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS 
WPSHtmlSWDefault(
	PPARSER_IN		in, 
	PPARSER_OUT		out);

GSTATUS
WPSHtmlLibName(
    PPARSER_IN  in,
    PPARSER_OUT out);

GSTATUS
WPSXmlTypeRaw ( PPARSER_IN in, PPARSER_OUT out);

GSTATUS
WPSXmlTypePdata( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSMarkupText( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXmlMarkupVar( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXmlTypeXml( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXmlTypeXmlPdata( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXMLSetBegin( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXMLSetEnd( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXMLSetElementBegin( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXMLSetElementEnd( PPARSER_IN in, PPARSER_OUT out );

STATUS
WPSXMLSetChild( PPARSER_IN in, PPARSER_OUT out );

GSTATUS
WPSXMLSetCellImage( PPARSER_IN in, PPARSER_OUT out );

#endif
