/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <postpars.h>
/*
**
**  Name: postpars.c - Parser to replace variable inside a posted query
**
**  Description:
**		Parser declaration
**
**  History:
**	5-Feb-98 (marol01)
**	    created
**      08-Oct-98 (fanra01)
**          Updated the parsing of a query to execute node entry and buffer
**          completion functions.  Also add the inclusion of variable in a
**          dynamic statement.
**      18-Feb-1999 (fanra01)
**          Fix compiler warning.
**      22-Jun-2001 (fanra01)
**          sir 103096
**          Add linefeed, carriage return to allowable characters in a query
**          passed via an HTML variable.
**      21-May-2003 (fanra01)
**          Bug 110283
**          Replace functionality to include HTML variables in SQL queries
**          passed to ICE as HTML ii_query_statment variable.
**/

#define POST_SQL        0
#define POST_SQL_PARAM  POST_SQL + 6

PARSER_NODE post[] = {
/* +00 */   { EOS,      EOS,        POST_SQL,         POST_SQL+5,       FALSE, FALSE, WPSHtmlSql,         WPSHtmlExecute},
/* +01 */   { '\n',     '\n',       POST_SQL,         POST_SQL+5,       TRUE , TRUE , NULL,               NULL},
/* +02 */   { '\r',     '\r',       POST_SQL,         POST_SQL+5,       TRUE , TRUE , NULL,               NULL},
/* +03 */   { ':',      ':',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, FALSE, FALSE, WPSHtmlSql,         NULL},
/* +04 */   { ';',      ';',        POST_SQL,         POST_SQL+5,       FALSE, FALSE, WPSHtmlNewSqlQuery, NULL},
/* +05 */   { ' ',      'z',        POST_SQL,         POST_SQL+5,       TRUE , TRUE , NULL,               NULL},
/* POST_SQL_PARAM */
/* +00 */   { ':',      ':',        POST_SQL        , POST_SQL+5,       TRUE,  TRUE,  WPSHtmlSql,         NULL},
/* +01 */   { EOS,      EOS,        POST_SQL        , POST_SQL+5,       FALSE, FALSE, WPSHtmlSqlParam,    WPSHtmlExecute},
/* +02 */   { '_',      '_',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, FALSE, TRUE,  NULL,               NULL},
/* +03 */   { '#',      '#',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, FALSE, TRUE,  NULL,               NULL},
/* +04 */   { 'A',      'Z',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, FALSE, TRUE,  NULL,               NULL},
/* +05 */   { '0',      '9',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, TRUE,  TRUE,  NULL,               NULL},
/* +06 */   { ':',      ':',        POST_SQL_PARAM+1, POST_SQL_PARAM+8, TRUE,  TRUE,  WPSHtmlSqlParam,    NULL},
/* +07 */   { ';',      ';',        POST_SQL        , POST_SQL+5,       TRUE,  TRUE,  WPSHtmlNewSqlQuery, NULL},
/* +08 */   { (u_i1)0, (u_i1)255,   POST_SQL        , POST_SQL+5,       TRUE,  FALSE, WPSHtmlSqlParam,    NULL},
};
