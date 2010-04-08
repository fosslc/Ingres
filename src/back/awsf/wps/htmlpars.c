/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <htmlpars.h>

/*
**
**  Name: htmlpars.c - HTML parser definition
**
**  Description:
**        Define the macro syntaxe
**
** History:
**      5-Feb-98 (marol01)
**          created
**      10-Sep-98 (marol01) (scott paley request)
**          force type = UNFORMATTED when the html tag is found.
**      20-Oct-98 (fanra01)
**          Update function for parsing sql statement containing parameters.
**      28-Oct-98 (fanra01)
**          Update the target nodes for retrieving the library name for
**          the function key word.  Replace the node function with
**          WPSHtmlLibName.
**      02-Nov-98 (fanra01)
**          Correct the target node for rollback keyword.  Add functions for
**          returning number of nodes and displaying a node entry.
**      04-Jan-2000 (fanra01)
**          Bug 99925
**          Modified to ignore whitespace around = in the SQL keyword.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Oct-2000 (fanra01)
**          Bug 102685
**          Update the keywords to ignore white space around the = character.
**          Setting separator flag to FALSE means space characters are skipped.
**          Add index comments to tie up with the #defines to try and make it
**          easier to add nodes in the future.
**      12-Oct-2000 (fanra01)
**          Sir 103096
**          Add parser entries for XML option following the SQL keyword.
**          Based on the HTML option.
**      02-Dec-2000 (fanra01)
**          Bug 102857
**          Modified double colon processing within declare keyword to
**          correctly set the value of the variable.
**          Also modified tag introducers and tag name separator so that
**          <code><@__#ice_sql=`select col1 from table`--></code> is permitted.
**      12-Jun-2001 (fanra01)
**          Sir 103096
**          Add auto-generation of XML tag result set.
**/


#define HTML_COMMENT        HTML_FIRST_NODE + 1
#define HTML_ICE            HTML_COMMENT + 12
#define HTML_ICE_PARAM      HTML_ICE + 7
#define HTML_TYPE           HTML_ICE_PARAM + 17
#define HTML_TYPE_LINK      HTML_TYPE + 14
#define HTML_TYPE_OLIST     HTML_TYPE_LINK + 3
#define HTML_TYPE_ULIST     HTML_TYPE_OLIST + 4
#define HTML_TYPE_COMBO     HTML_TYPE_ULIST + 5
#define HTML_TYPE_TABLE     HTML_TYPE_COMBO + 7
#define HTML_TYPE_IMAGE     HTML_TYPE_TABLE + 4
#define HTML_TYPE_RAW       HTML_TYPE_IMAGE + 4
#define HTML_TYPE_XML       HTML_TYPE_RAW + 9
#define HTML_SQL            HTML_TYPE_XML + 8
#define HTML_DATABASE       HTML_SQL + 18
#define HTML_ATTR           HTML_DATABASE + 12
#define HTML_LINKS          HTML_ATTR + 7
#define HTML_HEADERS        HTML_LINKS + 11
#define HTML_USER           HTML_HEADERS + 14
#define HTML_PASSWORD       HTML_USER + 7
#define HTML_VAR            HTML_PASSWORD + 11
#define HTML_EXT            HTML_VAR +21
#define HTML_NULL           HTML_EXT + 6
#define HTML_END            HTML_NULL + 10
#define HTML_HTML           HTML_END + 2        /* ICE 2.5 */
#define HTML_INCLUDE        HTML_HTML + 20      /* ICE 2.5 */
#define HTML_DECLARE        HTML_INCLUDE + 40   /* ICE 2.5 */
#define HTML_SERVER         HTML_DECLARE + 29   /* ICE 2.5 */
#define HTML_REPEAT         HTML_SERVER + 46    /* ICE 2.5 */
#define HTML_IF             HTML_REPEAT + 6     /* ICE 2.5 */
#define HTML_IF_THEN        HTML_IF + 43        /* ICE 2.5 */
#define HTML_IF_ELSE        HTML_IF_THEN + 21   /* ICE 2.5 */
#define HTML_IF_DEF         HTML_IF_ELSE + 24   /* ICE 2.5 */
#define HTML_TRANS          HTML_IF_DEF + 9     /* ICE 2.5 */
#define HTML_COMMIT         HTML_TRANS + 13     /* ICE 2.5 */
#define HTML_ROLLBACK       HTML_COMMIT + 11    /* ICE 2.5 */
#define HTML_CURSOR         HTML_ROLLBACK + 12  /* ICE 2.5 */
#define HTML_ROWS           HTML_CURSOR + 8     /* ICE 2.5 */
#define HTML_SWITCH         HTML_ROWS + 5       /* ICE 2.5 */
#define HTML_CASE           HTML_SWITCH + 23    /* ICE 2.5 */
#define HTML_DEFAULT        HTML_CASE + 41      /* ICE 2.5 */
#define HTML_XML            HTML_DEFAULT + 26   /* ICE 2.5 */
#define HTML_END_PARSER     HTML_XML + 26

PARSER_NODE html[] = {
/* +00 */   { '<',      '<',               HTML_COMMENT,      HTML_COMMENT+2,   TRUE , FALSE, NULL, WPSHtmlCopy},
/* +01 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE , TRUE , NULL, WPSHtmlCopy},

/*************************** HTML_COMMENT ***************************/
/* 002 */
/* +00 */   { '!',      '!',               HTML_COMMENT+3,    HTML_COMMENT+5,   TRUE,  TRUE , NULL, NULL},
/* +01 */   { '@',      '@',               HTML_COMMENT+3,    HTML_COMMENT+5,   TRUE,  TRUE , NULL, NULL},
/* +02 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE,  TRUE , NULL, NULL},
/* +03 */   { '-',      '-',               HTML_COMMENT+6,    HTML_COMMENT+7,   TRUE,  TRUE , NULL, NULL},
/* +04 */   { '_',      '_',               HTML_COMMENT+8,    HTML_COMMENT+9,   TRUE,  TRUE , NULL, NULL},
/* +05 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE,  TRUE , NULL, NULL},
/* +06 */   { '-',      '-',               HTML_COMMENT+10,   HTML_COMMENT+11,  TRUE,  FALSE, NULL, NULL},
/* +07 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE,  TRUE , NULL, NULL},
/* +08 */   { '_',      '_',               HTML_COMMENT+10,   HTML_COMMENT+11,  TRUE,  FALSE, NULL, NULL},
/* +09 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE,  TRUE , NULL, NULL},
/* +10 */   { '#',      '#',               HTML_ICE,          HTML_ICE+1,       TRUE,  TRUE , NULL, NULL},
/* +11 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE,  TRUE , NULL, NULL},

/*************************** HTML_ICE ***************************/
/* 014 */
/* +00 */   { 'I',     'I',                HTML_ICE+2,        HTML_ICE+3,       FALSE, TRUE , NULL, NULL},
/* +01 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE , TRUE , NULL, NULL},
/* +02 */   { 'C',     'C',                HTML_ICE+4,        HTML_ICE+5,       FALSE, TRUE , NULL, NULL},
/* +03 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE , TRUE , NULL, NULL},
/* +04 */   { 'E',     'E',                HTML_ICE+6,        HTML_TYPE-2,      FALSE, FALSE, WPSHtmlICE, NULL},
/* +05 */   { (char)0, (unsigned char)255, HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE , TRUE , NULL, NULL},
/* +06 */   { '_',     '_',                HTML_ICE_PARAM,    HTML_TYPE-2,      FALSE, FALSE, WPSHtmlICE, NULL},

/*************************** HTML_ICE_PARAM ***************************/
/* 021 */
/* +00 */   { 'A',     'A',                HTML_ATTR,         HTML_ATTR,        FALSE, TRUE , NULL, NULL},
/* +01 */   { 'D',     'D',                HTML_DATABASE,     HTML_DATABASE+1,  FALSE, TRUE , NULL, NULL},
/* +02 */   { 'C',     'C',                HTML_COMMIT,       HTML_COMMIT+1,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'E',     'E',                HTML_EXT,          HTML_EXT,         FALSE, TRUE , NULL, NULL},
/* +04 */   { 'F',     'F',                HTML_SERVER,       HTML_SERVER,      FALSE, TRUE , NULL, NULL},
/* +05 */   { 'H',     'H',                HTML_HEADERS,      HTML_HEADERS+1,   FALSE, TRUE , NULL, NULL},
/* +06 */   { 'I',     'I',                HTML_INCLUDE,      HTML_INCLUDE+1,   FALSE, TRUE , NULL, NULL},
/* +07 */   { 'L',     'L',                HTML_LINKS,        HTML_LINKS,       FALSE, TRUE , NULL, NULL},
/* +08 */   { 'N',     'N',                HTML_NULL,         HTML_NULL+1,      FALSE, TRUE , NULL, NULL},
/* +09 */   { 'P',     'P',                HTML_PASSWORD,     HTML_PASSWORD,    FALSE, TRUE , NULL, NULL},
/* +10 */   { 'R',     'R',                HTML_REPEAT,       HTML_REPEAT+1,    FALSE, TRUE , NULL, NULL},
/* +11 */   { 'S',     'S',                HTML_SQL ,         HTML_SQL+1,       FALSE, TRUE , NULL, NULL},
/* +12 */   { 'T',     'T',                HTML_TYPE,         HTML_TYPE+1,      FALSE, TRUE , NULL, NULL},
/* +13 */   { 'U',     'U',                HTML_USER,         HTML_USER,        FALSE, TRUE , NULL, NULL},
/* +14 */   { 'V',     'V',                HTML_VAR ,         HTML_VAR,         FALSE, TRUE , NULL, NULL},
/* +15 */   { 'X',     'X',                HTML_XML ,         HTML_XML,         FALSE, TRUE , NULL, NULL},
/* +16 */   { '-',     '-',                HTML_END ,         HTML_END,         TRUE , TRUE , WPSHtmlExecute, NULL},

/*************************** HTML_TYPE ***************************/
/* +00 */   { 'R',     'R',                HTML_TRANS,        HTML_TRANS,       FALSE, TRUE , NULL, NULL},
/* +01 */   { 'Y',     'Y',                HTML_TYPE+2,       HTML_TYPE+2,      FALSE, TRUE , NULL, NULL},
/* +02 */   { 'P',     'P',                HTML_TYPE+3,       HTML_TYPE+3,      FALSE, TRUE , NULL, NULL},
/* +03 */   { 'E',     'E',                HTML_TYPE+4,       HTML_TYPE+4,      FALSE, FALSE, NULL, NULL},
/* +04 */   { '=',     '=',                HTML_TYPE+5,       HTML_TYPE+5,      TRUE , FALSE, NULL, NULL},
/* +05 */   { '`',     '`',                HTML_TYPE+6,       HTML_TYPE+13,     TRUE , TRUE , NULL, NULL},
/* +06 */   { 'L',     'L',                HTML_TYPE_LINK,    HTML_TYPE_LINK+1, FALSE, TRUE , NULL, NULL},
/* +07 */   { 'O',     'O',                HTML_TYPE_OLIST,   HTML_TYPE_OLIST,  FALSE, TRUE , NULL, NULL},
/* +08 */   { 'P',     'P',                HTML_TYPE_IMAGE,   HTML_TYPE_IMAGE,  FALSE, TRUE , NULL, NULL},
/* +09 */   { 'S',     'S',                HTML_TYPE_COMBO,   HTML_TYPE_COMBO,  FALSE, TRUE , NULL, NULL},
/* +10 */   { 'T',     'T',                HTML_TYPE_TABLE,   HTML_TYPE_TABLE,  FALSE, TRUE , NULL, NULL},
/* +11 */   { 'U',     'U',                HTML_TYPE_ULIST,   HTML_TYPE_ULIST+1,FALSE, TRUE , NULL, NULL},
/* +12 */   { 'X',     'X',                HTML_TYPE_XML,     HTML_TYPE_XML,    FALSE, TRUE , NULL, NULL},
/* +13 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_TYPE_LINK ***************************/
/* +00 */   { 'I',     'I',                HTML_TYPE_LINK+2,  HTML_TYPE_LINK+2, FALSE, TRUE , NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_TYPE_LINK+2,  HTML_TYPE_LINK+2, FALSE, TRUE , NULL, NULL},
/* +02 */   { 'K',     'K',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK-1, FALSE, FALSE, WPSHtmlTypeLink, NULL},

/*************************** HTML_TYPE_OLIST ***************************/
/* +00 */   { 'L',     'L',                HTML_TYPE_OLIST+1, HTML_TYPE_OLIST+1,FALSE, TRUE , NULL, NULL},
/* +01 */   { 'I',     'I',                HTML_TYPE_OLIST+2, HTML_TYPE_OLIST+2,FALSE, TRUE , NULL, NULL},
/* +02 */   { 'S',     'S',                HTML_TYPE_OLIST+3, HTML_TYPE_OLIST+3,FALSE, TRUE , NULL, NULL},
/* +03 */   { 'T',     'T',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK -1,FALSE, FALSE, WPSHtmlTypeOList, NULL},

/*************************** HTML_TYPE_ULIST ***************************/
/* +00 */   { 'L',     'L',                HTML_TYPE_ULIST+2, HTML_TYPE_ULIST+2,FALSE, TRUE , NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_TYPE_RAW ,    HTML_TYPE_RAW,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'I',     'I',                HTML_TYPE_ULIST+3, HTML_TYPE_ULIST+3,FALSE, TRUE , NULL, NULL},
/* +03 */   { 'S',     'S',                HTML_TYPE_ULIST+4, HTML_TYPE_ULIST+4,FALSE, TRUE , NULL, NULL},
/* +04 */   { 'T',     'T',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK -1,FALSE, FALSE, WPSHtmlTypeUList, NULL},

/*************************** HTML_TYPE_COMBO ***************************/
/* +00 */   { 'E',     'E',                HTML_TYPE_COMBO+1, HTML_TYPE_COMBO+1,FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_TYPE_COMBO+2, HTML_TYPE_COMBO+2,FALSE, TRUE , NULL, NULL},
/* +02 */   { 'E',     'E',                HTML_TYPE_COMBO+3, HTML_TYPE_COMBO+3,FALSE, TRUE , NULL, NULL},
/* +03 */   { 'C',     'C',                HTML_TYPE_COMBO+4, HTML_TYPE_COMBO+4,FALSE, TRUE , NULL, NULL},
/* +04 */   { 'T',     'T',                HTML_TYPE_COMBO+5, HTML_TYPE_COMBO+5,FALSE, TRUE , NULL, NULL},
/* +05 */   { 'O',     'O',                HTML_TYPE_COMBO+6, HTML_TYPE_COMBO+6,FALSE, TRUE , NULL, NULL},
/* +06 */   { 'R',     'R',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK-1, FALSE, FALSE, WPSHtmlTypeCombo, NULL},
/*************************** HTML_TYPE_TABLE ***************************/
/* +00 */   { 'A',     'A',                HTML_TYPE_TABLE+1, HTML_TYPE_TABLE+1,FALSE, TRUE , NULL, NULL},
/* +01 */   { 'B',     'B',                HTML_TYPE_TABLE+2, HTML_TYPE_TABLE+2,FALSE, TRUE , NULL, NULL},
/* +02 */   { 'L',     'L',                HTML_TYPE_TABLE+3, HTML_TYPE_TABLE+3,FALSE, TRUE , NULL, NULL},
/* +03 */   { 'E',     'E',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK-1, FALSE, FALSE, WPSHtmlTypeTable, NULL},
/*************************** HTML_TYPE_IMAGE ***************************/
/* +00 */   { 'L',     'L',                HTML_TYPE_IMAGE+1, HTML_TYPE_IMAGE+1,FALSE, TRUE , NULL, NULL},
/* +01 */   { 'A',     'A',                HTML_TYPE_IMAGE+2, HTML_TYPE_IMAGE+2,FALSE, TRUE , NULL, NULL},
/* +02 */   { 'I',     'I',                HTML_TYPE_IMAGE+3, HTML_TYPE_IMAGE+3,FALSE, TRUE , NULL, NULL},
/* +03 */   { 'N',     'N',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK -1,FALSE, FALSE, WPSHtmlTypeImage, NULL},
/*************************** HTML_TYPE_RAW ***************************/
/* +00 */   { 'F',     'F',                HTML_TYPE_RAW+1,   HTML_TYPE_RAW+1,  FALSE, TRUE , NULL, NULL},
/* +01 */   { 'O',     'O',                HTML_TYPE_RAW+2,   HTML_TYPE_RAW+2,  FALSE, TRUE , NULL, NULL},
/* +02 */   { 'R',     'R',                HTML_TYPE_RAW+3,   HTML_TYPE_RAW+3,  FALSE, TRUE , NULL, NULL},
/* +03 */   { 'M',     'M',                HTML_TYPE_RAW+4,   HTML_TYPE_RAW+4,  FALSE, TRUE , NULL, NULL},
/* +04 */   { 'A',     'A',                HTML_TYPE_RAW+5,   HTML_TYPE_RAW+5,  FALSE, TRUE , NULL, NULL},
/* +05 */   { 'T',     'T',                HTML_TYPE_RAW+6,   HTML_TYPE_RAW+6,  FALSE, TRUE , NULL, NULL},
/* +06 */   { 'T',     'T',                HTML_TYPE_RAW+7,   HTML_TYPE_RAW+7,  FALSE, TRUE , NULL, NULL},
/* +07 */   { 'E',     'E',                HTML_TYPE_RAW+8,   HTML_TYPE_RAW+8,  FALSE, TRUE , NULL, NULL},
/* +08 */   { 'D',     'D',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK-1, FALSE, FALSE, WPSHtmlTypeRaw, NULL},
/*************************** HTML_TYPE_XML ***************************/
/* +00 */   { 'M',     'M',                HTML_TYPE_XML+1,   HTML_TYPE_XML+1,  FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_TYPE_XML+2,   HTML_TYPE_XML+3,  FALSE, FALSE, WPSXmlTypeXml, NULL},
/* +02 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +03 */   { 'P',     'P',                HTML_TYPE_XML+4,   HTML_TYPE_XML+4,  FALSE, FALSE, NULL, NULL},
/* +04 */   { 'D',     'D',                HTML_TYPE_XML+5,   HTML_TYPE_XML+5,  FALSE, FALSE, NULL, NULL},
/* +05 */   { 'A',     'A',                HTML_TYPE_XML+6,   HTML_TYPE_XML+6,  FALSE, FALSE, NULL, NULL},
/* +06 */   { 'T',     'T',                HTML_TYPE_XML+7,   HTML_TYPE_XML+7,  FALSE, FALSE, NULL, NULL},
/* +07 */   { 'A',     'A',                HTML_TYPE_LINK-1,  HTML_TYPE_LINK-1, FALSE, FALSE, WPSXmlTypeXmlPdata, NULL},

/*************************** HTML_SQL ***************************/
/* +00 */   { 'W',     'W',                HTML_SWITCH,       HTML_SWITCH,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'Q',     'Q',                HTML_SQL+2,        HTML_SQL+2,       FALSE, TRUE , NULL, NULL},
/* +02 */   { 'L',     'L',                HTML_SQL+3,        HTML_SQL+3,       FALSE, FALSE, NULL, NULL},
/* +03 */   { '=',     '=',                HTML_SQL+4,        HTML_SQL+4,       TRUE , FALSE, NULL, NULL},
/* +04 */   { '`',     '`',                HTML_SQL+5,        HTML_SQL+8,       TRUE , FALSE, WPSHtmlNewSqlQuery, NULL},
/* +05 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlSql, NULL},
/* +06 */   { ':',     ':',                HTML_SQL+9,        HTML_SQL+17,      TRUE , TRUE , WPSHtmlSqlParam, NULL},
/* +07 */   { ';',     ';',                HTML_SQL+5,        HTML_SQL+8,       TRUE , FALSE, WPSHtmlNewSqlQuery, NULL},
/* +08 */   { (char)0, (unsigned char)255, HTML_SQL+5,        HTML_SQL+8,       TRUE , FALSE, NULL, NULL},
/* +09 */   { ':',     ':',                HTML_SQL+5,        HTML_SQL+8,       TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +10 */   { '_',     '_',                HTML_SQL+10,       HTML_SQL+17,      FALSE, TRUE , NULL, NULL},
/* +11 */   { '#',     '#',                HTML_SQL+10,       HTML_SQL+17,      FALSE, TRUE , NULL, NULL},
/* +12 */   { 'A',     'Z',                HTML_SQL+10,       HTML_SQL+17,      FALSE, TRUE , NULL, NULL},
/* +13 */   { '0',     '9',                HTML_SQL+10,       HTML_SQL+17,      TRUE , TRUE , NULL, NULL},
/* +14 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlSqlParam, NULL},
/* +15 */   { ':',     ':',                HTML_SQL+10,       HTML_SQL+17,      TRUE , TRUE , WPSHtmlSqlParam, NULL},
/* +16 */   { ';',     ';',                HTML_SQL+5,        HTML_SQL+8,       TRUE , TRUE , WPSHtmlNewSqlQuery, NULL},
/* +17 */   { (char)0, (unsigned char)255, HTML_SQL+5,        HTML_SQL+8,       TRUE , FALSE, WPSHtmlSqlParam, NULL},

/*************************** HTML_DATABASE ***************************/
/* +00 */   { 'A',     'A',                HTML_DATABASE+2,   HTML_DATABASE+2,  FALSE, TRUE , NULL, NULL},
/* +01 */   { 'E',     'E',                HTML_DECLARE,      HTML_DECLARE,     FALSE, TRUE , NULL, NULL},
/* +02 */   { 'T',     'T',                HTML_DATABASE+3,   HTML_DATABASE+3,  FALSE, TRUE , NULL, NULL},
/* +03 */   { 'A',     'A',                HTML_DATABASE+4,   HTML_DATABASE+4,  FALSE, TRUE , NULL, NULL},
/* +04 */   { 'B',     'B',                HTML_DATABASE+5,   HTML_DATABASE+5,  FALSE, TRUE , NULL, NULL},
/* +05 */   { 'A',     'A',                HTML_DATABASE+6,   HTML_DATABASE+6,  FALSE, TRUE , NULL, NULL},
/* +06 */   { 'S',     'S',                HTML_DATABASE+7,   HTML_DATABASE+7,  FALSE, TRUE , NULL, NULL},
/* +07 */   { 'E',     'E',                HTML_DATABASE+8,   HTML_DATABASE+8,  FALSE, FALSE, NULL, NULL},
/* +08 */   { '=',     '=',                HTML_DATABASE+9,   HTML_DATABASE+9,  TRUE , FALSE, NULL, NULL},
/* +09 */   { '`',     '`',                HTML_DATABASE+10,  HTML_DATABASE+11, TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +10 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE -1,     TRUE , FALSE, WPSHtmlDatabase, NULL},
/* +11 */   { (char)0, (unsigned char)255, HTML_DATABASE+10,  HTML_DATABASE+11, TRUE , FALSE, NULL, NULL},

/*************************** HTML_ATTR ***************************/
/* +00 */   { 'T',     'T',                HTML_ATTR+1,       HTML_ATTR+1,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'T',     'T',                HTML_ATTR+2,       HTML_ATTR+2,      FALSE, TRUE , NULL, NULL},
/* +02 */   { 'R',     'R',                HTML_ATTR+3,       HTML_ATTR+3,      FALSE, FALSE, NULL, NULL},
/* +03 */   { '=',     '=',                HTML_ATTR+4,       HTML_ATTR+4,      TRUE , FALSE, NULL, NULL},
/* +04 */   { '`',     '`',                HTML_ATTR+5,       HTML_ATTR+6,      TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +05 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlAttr    , NULL},
/* +06 */   { (char)0, (unsigned char)255, HTML_ATTR+5,       HTML_ATTR+6,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_LINKS ***************************/
/* +00 */   { 'I',     'I',                HTML_LINKS+1,      HTML_LINKS+1,     FALSE, TRUE , NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_LINKS+2,      HTML_LINKS+2,     FALSE, TRUE , NULL, NULL},
/* +02 */   { 'K',     'K',                HTML_LINKS+3,      HTML_LINKS+3,     FALSE, TRUE , NULL, NULL},
/* +03 */   { 'S',     'S',                HTML_LINKS+4,      HTML_LINKS+4,     FALSE, FALSE, NULL, NULL},
/* +04 */   { '=',     '=',                HTML_LINKS+5,      HTML_LINKS+5,     TRUE , FALSE, NULL, NULL},
/* +05 */   { '`',     '`',                HTML_LINKS+6,      HTML_LINKS+7,     TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +06 */   { ',',     ',',                HTML_LINKS+8,      HTML_LINKS+10,    TRUE , FALSE, WPSHtmllinksName, NULL},
/* +07 */   { (char)0, (unsigned char)255, HTML_LINKS+6,      HTML_LINKS+7,     TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE -1,     TRUE , FALSE, WPSHtmllinksData, NULL},
/* +09 */   { ',',     ',',                HTML_LINKS+6,      HTML_LINKS+7,     TRUE , FALSE, WPSHtmllinksData, NULL},
/* +10 */   { (char)0, (unsigned char)255, HTML_LINKS+8,      HTML_LINKS+10,    TRUE , FALSE, NULL, NULL},

/*************************** HTML_HEADERS ***************************/
/* +00 */   { 'E',     'E',                HTML_HEADERS+2,    HTML_HEADERS+2,   FALSE, TRUE , NULL, NULL},
/* +01 */   { 'T',     'T',                HTML_HTML,         HTML_HTML,        FALSE, TRUE , NULL, NULL},
/* +02 */   { 'A',     'A',                HTML_HEADERS+3,    HTML_HEADERS+3,   FALSE, TRUE , NULL, NULL},
/* +03 */   { 'D',     'D',                HTML_HEADERS+4,    HTML_HEADERS+4,   FALSE, TRUE , NULL, NULL},
/* +04 */   { 'E',     'E',                HTML_HEADERS+5,    HTML_HEADERS+5,   FALSE, FALSE, NULL, NULL},
/* +05 */   { 'R',     'R',                HTML_HEADERS+6,    HTML_HEADERS+6,   FALSE, FALSE, NULL, NULL},
/* +06 */   { 'S',     'S',                HTML_HEADERS+7,    HTML_HEADERS+7,   FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_HEADERS+8,    HTML_HEADERS+8,   TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_HEADERS+9,    HTML_HEADERS+10,  TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { ',',     ',',                HTML_HEADERS+11,   HTML_HEADERS+13,  TRUE , FALSE, WPSHtmlheadersName, NULL},
/* +10 */   { (char)0, (unsigned char)255, HTML_HEADERS+9,    HTML_HEADERS+10,  TRUE , FALSE, NULL, NULL},
/* +11 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlheadersData, NULL},
/* +12 */   { ',',     ',',                HTML_HEADERS+9,    HTML_HEADERS+10,  TRUE , FALSE, WPSHtmlheadersData, NULL},
/* +13 */   { (char)0, (unsigned char)255, HTML_HEADERS+11,   HTML_HEADERS+13,  TRUE , FALSE, NULL, NULL},

/*************************** HTML_USER ***************************/
/* +00 */   { 'S',     'S',                HTML_USER+1,       HTML_USER+1,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'E',     'E',                HTML_USER+2,       HTML_USER+2,      FALSE, TRUE , NULL, NULL},
/* +02 */   { 'R',     'R',                HTML_USER+3,       HTML_USER+3,      FALSE, FALSE, NULL, NULL},
/* +03 */   { '=',     '=',                HTML_USER+4,       HTML_USER+4,      TRUE , FALSE, NULL, NULL},
/* +04 */   { '`',     '`',                HTML_USER+5,       HTML_USER+6,      TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +05 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlUser, NULL},
/* +06 */   { (char)0, (unsigned char)255, HTML_USER+5,       HTML_USER+6,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_PASSWORD ***************************/
/* +00 */   { 'A',     'A',                HTML_PASSWORD+1,   HTML_PASSWORD+1,  FALSE, TRUE,  NULL, NULL},
/* +01 */   { 'S',     'S',                HTML_PASSWORD+2,   HTML_PASSWORD+2,  FALSE, TRUE,  NULL, NULL},
/* +02 */   { 'S',     'S',                HTML_PASSWORD+3,   HTML_PASSWORD+3,  FALSE, FALSE, NULL, NULL},
/* +03 */   { 'W',     'W',                HTML_PASSWORD+4,   HTML_PASSWORD+4,  FALSE, FALSE, NULL, NULL},
/* +04 */   { 'O',     'O',                HTML_PASSWORD+5,   HTML_PASSWORD+5,  FALSE, FALSE, NULL, NULL},
/* +05 */   { 'R',     'R',                HTML_PASSWORD+6,   HTML_PASSWORD+6,  FALSE, FALSE, NULL, NULL},
/* +06 */   { 'D',     'D',                HTML_PASSWORD+7,   HTML_PASSWORD+7,  FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_PASSWORD+8,   HTML_PASSWORD+8,  TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_PASSWORD+9,   HTML_PASSWORD+10, TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlPassword, NULL},
/* +10 */   { (char)0, (unsigned char)255, HTML_PASSWORD+9,   HTML_PASSWORD+10, TRUE , FALSE, NULL, NULL},

/*************************** HTML_VAR ***************************/
/* +00 */   { 'A',     'A',                HTML_VAR+1,        HTML_VAR+1,       FALSE, TRUE , NULL, NULL},
/* +01 */   { 'R',     'R',                HTML_VAR+2,        HTML_VAR+2,       FALSE, FALSE, NULL, NULL},
/* +02 */   { '=',     '=',                HTML_VAR+3,        HTML_VAR+3,       TRUE , FALSE, NULL, NULL},
/* +03 */   { '`',     '`',                HTML_VAR+4,        HTML_VAR+6,       TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +04 */   { '`',     '`',                HTML_VAR+15,       HTML_VAR+20,      TRUE , TRUE , WPSHtmlVar, NULL},
/* +05 */   { ':',     ':',                HTML_VAR+7,        HTML_VAR+14,      TRUE , TRUE , WPSHtmlVar, NULL},
/* +06 */   { (char)0, (unsigned char)255, HTML_VAR+4,        HTML_VAR+6,       TRUE , FALSE, NULL, NULL},
/* +07 */   { ':',     ':',                HTML_VAR+4,        HTML_VAR+6,       TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +08 */   { '_',     '_',                HTML_VAR+8,        HTML_VAR+14,      TRUE , TRUE , NULL, NULL},
/* +09 */   { '#',     '#',                HTML_VAR+8,        HTML_VAR+14,      TRUE , TRUE , NULL, NULL},
/* +10 */   { 'A',     'Z',                HTML_VAR+8,        HTML_VAR+14,      FALSE, TRUE , NULL, NULL},
/* +11 */   { '0',     '9',                HTML_VAR+8,        HTML_VAR+14,      TRUE , TRUE , NULL, NULL},
/* +12 */   { '`',     '`',                HTML_VAR+15,       HTML_VAR+20,      TRUE , FALSE, WPSHtmlVar, NULL},
/* +13 */   { ':',     ':',                HTML_VAR+8,        HTML_VAR+14,      TRUE , TRUE , WPSHtmlVar, NULL},
/* +14 */   { (char)0, (unsigned char)255, HTML_VAR+4,        HTML_VAR+6,       TRUE , FALSE, WPSHtmlVar, NULL},
/* +15 */   { '`',     '`',                HTML_VAR+4,        HTML_VAR+6,       TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +16 */   { ' ',     ' ',                HTML_VAR+20,       HTML_VAR+20,      TRUE , FALSE, NULL, NULL},
/* +17 */   { '\n',    '\n',               HTML_VAR+20,       HTML_VAR+20,      TRUE , FALSE, NULL, NULL},
/* +18 */   { '\r',    '\r',               HTML_VAR+20,       HTML_VAR+20,      TRUE , FALSE, NULL, NULL},
/* +19 */   { '\t',    '\t',               HTML_VAR+20,       HTML_VAR+20,      TRUE , FALSE, NULL, NULL},
/* +20 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , NULL, NULL},

/*************************** HTML_EXT ***************************/
/* +00 */   { 'X',     'X',                HTML_EXT+1,        HTML_EXT+1,       FALSE, TRUE , NULL, NULL},
/* +01 */   { 'T',     'T',                HTML_EXT+2,        HTML_EXT+2,       FALSE, FALSE, NULL, NULL},
/* +02 */   { '=',     '=',                HTML_EXT+3,        HTML_EXT+3,       TRUE , FALSE, NULL, NULL},
/* +03 */   { '`',     '`',                HTML_EXT+4,        HTML_EXT+5,       TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +04 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlExtension, NULL},
/* +05 */   { (char)0, (unsigned char)255, HTML_EXT+4,        HTML_EXT+5,       TRUE , FALSE, NULL, NULL},

/*************************** HTML_NULL ***************************/
/* +00 */   { 'U',     'U',                HTML_NULL+1,       HTML_NULL+1,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_NULL+2,       HTML_NULL+2,      FALSE, TRUE , NULL, NULL},
/* +02 */   { 'L',     'L',                HTML_NULL+3,       HTML_NULL+3,      FALSE, TRUE , NULL, NULL},
/* +03 */   { 'V',     'V',                HTML_NULL+4,       HTML_NULL+4,      FALSE, TRUE , NULL, NULL},
/* +04 */   { 'A',     'A',                HTML_NULL+5,       HTML_NULL+5,      FALSE, TRUE , NULL, NULL},
/* +05 */   { 'R',     'R',                HTML_NULL+6,       HTML_NULL+6,      FALSE, TRUE , NULL, NULL},
/* +06 */   { '=',     '=',                HTML_NULL+7,       HTML_NULL+7,      TRUE , FALSE, NULL, NULL},
/* +07 */   { '`',     '`',                HTML_NULL+8,       HTML_NULL+9,      TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +08 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlNullVar, NULL},
/* +09 */   { (char)0, (unsigned char)255, HTML_NULL+8,       HTML_NULL+9,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_END ***************************/
/* +00 */   { '-',     '-',                HTML_END+1,        HTML_END+1,       TRUE , TRUE , NULL, NULL},
/* +01 */   { '>',     '>',                HTML_FIRST_NODE-1, HTML_FIRST_NODE,  TRUE , FALSE, WPSHtmlEnd, WPSHtmlEnd},

/*************************** HTML_HTML ***************************/
/* +00 */   { 'M',     'M',                HTML_HTML+1,       HTML_HTML+1,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_HTML+2,       HTML_HTML+2,      FALSE, FALSE, WPSHtmlTypeRaw, NULL},
/* +02 */   { '=',     '=',                HTML_HTML+3,       HTML_HTML+3,      TRUE , FALSE, NULL, NULL},
/* +03 */   { '`',     '`',                HTML_HTML+4,       HTML_HTML+6,      TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +04 */   { '`',     '`',                HTML_HTML+15,      HTML_HTML+19,     TRUE , TRUE , WPSHtmlHtmlText, NULL},
/* +05 */   { ':',     ':',                HTML_HTML+7,       HTML_HTML+14,     TRUE , TRUE , WPSHtmlHtmlText, NULL},
/* +06 */   { (char)0, (unsigned char)255, HTML_HTML+4,       HTML_HTML+6,      TRUE , FALSE, NULL, NULL},
/* +07 */   { ':',     ':',                HTML_HTML+4,       HTML_HTML+6,      TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +08 */   { '_',     '_',                HTML_HTML+8,       HTML_HTML+14,     TRUE , TRUE , NULL, NULL},
/* +09 */   { '#',     '#',                HTML_HTML+8,       HTML_HTML+14,     TRUE , TRUE , NULL, NULL},
/* +10 */   { 'A',     'Z',                HTML_HTML+8,       HTML_HTML+14,     FALSE, TRUE , NULL, NULL},
/* +11 */   { '0',     '9',                HTML_HTML+8,       HTML_HTML+14,     TRUE , TRUE , NULL, NULL},
/* +12 */   { '`',     '`',                HTML_HTML+15,      HTML_HTML+19,     TRUE , TRUE , WPSHtmlHtmlVar, NULL},
/* +13 */   { ':',     ':',                HTML_HTML+8,       HTML_HTML+14,     TRUE , TRUE , WPSHtmlHtmlVar, NULL},
/* +14 */   { (char)0, (unsigned char)255, HTML_HTML+4,       HTML_HTML+6,      TRUE , FALSE, WPSHtmlHtmlVar, NULL},
/* +15 */   { '`',     '`',                HTML_HTML+4,       HTML_HTML+6,      TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +16 */   { ' ',     ' ',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +17 */   { '\n',    '\n',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +18 */   { '\r',    '\r',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +19 */   { '\t',    '\t',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_INCLUDE ***************************/
/* +00 */   { 'F',     'F',                HTML_IF,           HTML_IF+2,        FALSE, FALSE, NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_INCLUDE+2,    HTML_INCLUDE+2,   FALSE, TRUE , NULL, NULL},
/* +02 */   { 'C',     'C',                HTML_INCLUDE+3,    HTML_INCLUDE+3,   FALSE, TRUE , NULL, NULL},
/* +03 */   { 'L',     'L',                HTML_INCLUDE+4,    HTML_INCLUDE+4,   FALSE, TRUE , NULL, NULL},
/* +04 */   { 'U',     'U',                HTML_INCLUDE+5,    HTML_INCLUDE+5,   FALSE, TRUE , NULL, NULL},
/* +05 */   { 'D',     'D',                HTML_INCLUDE+6,    HTML_INCLUDE+6,   FALSE, TRUE , NULL, NULL},
/* +06 */   { 'E',     'E',                HTML_INCLUDE+7,    HTML_INCLUDE+7,   FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_INCLUDE+8,    HTML_INCLUDE+8,   TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_INCLUDE+9,    HTML_INCLUDE+11,  TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { '`',     '`',                HTML_INCLUDE+14,   HTML_INCLUDE+15,  TRUE , FALSE, WPSHtmlName25, NULL},
/* +00 */   { '?',     '?',                HTML_INCLUDE+12,   HTML_INCLUDE+13,  TRUE , TRUE , WPSHtmlName25, NULL},
/* +11 */   { (char)0, (unsigned char)255, HTML_INCLUDE+9,    HTML_INCLUDE+11,  TRUE , FALSE, NULL, NULL},
/* +12 */   { '`',     '`',                HTML_INCLUDE+14,   HTML_INCLUDE+15,  TRUE , FALSE, WPSHtmlValue25, NULL},
/* +13 */   { (char)0, (unsigned char)255, HTML_INCLUDE+12,   HTML_INCLUDE+13,  TRUE , FALSE, NULL,    NULL},
/* +14 */   { 'T',     'T',                HTML_INCLUDE+16,   HTML_INCLUDE+16,  FALSE, TRUE , NULL, NULL},
/* +15 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlInclude, NULL},
/* +16 */   { 'Y',     'Y',                HTML_INCLUDE+17,   HTML_INCLUDE+17,  FALSE, TRUE , NULL, NULL},
/* +17 */   { 'P',     'P',                HTML_INCLUDE+18,   HTML_INCLUDE+18,  FALSE, TRUE , NULL, NULL},
/* +18 */   { 'E',     'E',                HTML_INCLUDE+19,   HTML_INCLUDE+19,  FALSE, FALSE, NULL, NULL},
/* +19 */   { '=',     '=',                HTML_INCLUDE+20,   HTML_INCLUDE+20,  TRUE , FALSE, NULL, NULL},
/* +20 */   { '`',     '`',                HTML_INCLUDE+21,   HTML_INCLUDE+25,  TRUE , FALSE, NULL, NULL},
/* +21 */   { 'H',     'H',                HTML_INCLUDE+26,   HTML_INCLUDE+26,  FALSE, TRUE , NULL, NULL},
/* +22 */   { 'F',     'F',                HTML_INCLUDE+36,   HTML_INCLUDE+36,  FALSE, TRUE , NULL, NULL},
/* +23 */   { 'R',     'R',                HTML_INCLUDE+29,   HTML_INCLUDE+29,  FALSE, TRUE , NULL, NULL},
/* +24 */   { 'E',     'E',                HTML_INCLUDE+34,   HTML_INCLUDE+34,  FALSE, TRUE , NULL, NULL},
/* +25 */   { '`',     '`',                HTML_INCLUDE+15,   HTML_INCLUDE+15,  TRUE , FALSE, NULL, NULL},
/* +26 */   { 'T',     'T',                HTML_INCLUDE+27,   HTML_INCLUDE+27,  FALSE, TRUE , NULL, NULL},
/* +27 */   { 'M',     'M',                HTML_INCLUDE+28,   HTML_INCLUDE+28,  FALSE, TRUE , NULL, NULL},
/* +28 */   { 'L',     'L',                HTML_INCLUDE+25,   HTML_INCLUDE+25,  FALSE, FALSE, WPSHtmlIncHTML, NULL},

/* +29 */   { 'E',     'E',                HTML_INCLUDE+30,   HTML_INCLUDE+30,  FALSE, TRUE , NULL, NULL},
/* +30 */   { 'P',     'P',                HTML_INCLUDE+31,   HTML_INCLUDE+31,  FALSE, TRUE , NULL, NULL},
/* +31 */   { 'O',     'O',                HTML_INCLUDE+32,   HTML_INCLUDE+32,  FALSE, TRUE , NULL, NULL},
/* +32 */   { 'R',     'R',                HTML_INCLUDE+33,   HTML_INCLUDE+33,  FALSE, FALSE, NULL, NULL},
/* +33 */   { 'T',     'T',                HTML_INCLUDE+25,   HTML_INCLUDE+25,  FALSE, FALSE, WPSHtmlIncReport, NULL},

/* +34 */   { 'X',     'X',                HTML_INCLUDE+35,   HTML_INCLUDE+35,  FALSE, TRUE , NULL, NULL},
/* +35 */   { 'E',     'E',                HTML_INCLUDE+25,   HTML_INCLUDE+25,  FALSE, FALSE, WPSHtmlIncExe, NULL},

/* +36 */   { 'A',     'A',                HTML_INCLUDE+37,   HTML_INCLUDE+37,  FALSE, TRUE , NULL, NULL},
/* +37 */   { 'C',     'C',                HTML_INCLUDE+38,   HTML_INCLUDE+38,  FALSE, TRUE , NULL, NULL},
/* +38 */   { 'E',     'E',                HTML_INCLUDE+39,   HTML_INCLUDE+39,  FALSE, TRUE , NULL, NULL},
/* +39 */   { 'T',     'T',                HTML_INCLUDE+25,   HTML_INCLUDE+25,  FALSE, FALSE, WPSHtmlIncMulti, NULL},

/*************************** HTML_DECLARE ***************************/
/* +00 */   { 'C',     'C',                HTML_DECLARE+1,    HTML_DECLARE+1,   FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_DECLARE+2,    HTML_DECLARE+2,   FALSE, TRUE , NULL, NULL},
/* +02 */   { 'A',     'A',                HTML_DECLARE+3,    HTML_DECLARE+3,   FALSE, TRUE , NULL, NULL},
/* +03 */   { 'R',     'R',                HTML_DECLARE+4,    HTML_DECLARE+4,   FALSE, TRUE , NULL, NULL},
/* +04 */   { 'E',     'E',                HTML_DECLARE+5,    HTML_DECLARE+5,   FALSE, FALSE , NULL, NULL},
/* +05 */   { '=',     '=',                HTML_DECLARE+6,    HTML_DECLARE+6,   TRUE , FALSE, NULL, NULL},
/* +06 */   { '`',     '`',                HTML_DECLARE+7,    HTML_DECLARE+9,   TRUE , FALSE , WPSHtmlMoveAfter, NULL},
/* +07 */   { '.',     '.',                HTML_DECLARE+10,   HTML_DECLARE+11,  TRUE , TRUE , WPSHtmlDeclLevel, NULL},
/* +08 */   { '=',     '=',                HTML_DECLARE+12,   HTML_DECLARE+14,  TRUE , TRUE , WPSHtmlName25, NULL},
/* +09 */   { (char)0, (unsigned char)255, HTML_DECLARE+7,    HTML_DECLARE+9,   TRUE , FALSE, NULL, NULL},
/* +10 */   { '=',     '=',                HTML_DECLARE+12,   HTML_DECLARE+14,  TRUE , TRUE , WPSHtmlName25, NULL},
/* +11 */   { (char)0, (unsigned char)255, HTML_DECLARE+10,   HTML_DECLARE+11,  TRUE , FALSE, NULL,    NULL},
/* +12 */   { '`',     '`',                HTML_DECLARE+23,   HTML_DECLARE+28,  TRUE , TRUE , WPSHtmlValue25, NULL},
/* +13 */   { ':',     ':',                HTML_DECLARE+15,   HTML_DECLARE+22,  TRUE , TRUE , WPSHtmlValue25, NULL},
/* +14 */   { (char)0, (unsigned char)255, HTML_DECLARE+12,   HTML_DECLARE+14,  TRUE , FALSE, NULL, NULL},
/* +15 */   { ':',     ':',                HTML_DECLARE+13,   HTML_DECLARE+22,  TRUE , TRUE , WPSAppendCharToValue, NULL},
/* +16 */   { '_',     '_',                HTML_DECLARE+16,   HTML_DECLARE+22,  TRUE , TRUE , NULL, NULL},
/* +17 */   { '#',     '#',                HTML_DECLARE+16,   HTML_DECLARE+22,  TRUE , TRUE , NULL, NULL},
/* +18 */   { 'A',     'Z',                HTML_DECLARE+16,   HTML_DECLARE+22,  FALSE, TRUE , NULL, NULL},
/* +19 */   { '0',     '9',                HTML_DECLARE+16,   HTML_DECLARE+22,  TRUE , TRUE , NULL, NULL},
/* +20 */   { '`',     '`',                HTML_DECLARE+23,   HTML_DECLARE+28,  TRUE , TRUE , WPSHtmlValue25, NULL},
/* +21 */   { ':',     ':',                HTML_DECLARE+15,   HTML_DECLARE+22,  TRUE , TRUE , WPSHtmlValue25, NULL},
/* +22 */   { (char)0, (unsigned char)255, HTML_DECLARE+12,   HTML_DECLARE+14,  TRUE , FALSE, WPSHtmlValue25, NULL},
/* +23 */   { '`',     '`',                HTML_DECLARE+12,   HTML_DECLARE+14,  TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +24 */   { ' ',     ' ',                HTML_DECLARE+28,   HTML_DECLARE+28,  TRUE , FALSE, NULL, NULL},
/* +25 */   { '\n',    '\n',               HTML_DECLARE+28,   HTML_DECLARE+28,  TRUE , FALSE, NULL, NULL},
/* +26 */   { '\r',    '\r',               HTML_DECLARE+28,   HTML_DECLARE+28,  TRUE , FALSE, NULL, NULL},
/* +27 */   { '\t',    '\t',               HTML_DECLARE+28,   HTML_DECLARE+28,  TRUE , FALSE, NULL, NULL},
/* +28 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlDecl, NULL},

/*************************** HTML_SERVER ***************************/
/* +00 */   { 'U',     'U',                HTML_SERVER+1,     HTML_SERVER+1,    FALSE, TRUE , NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_SERVER+2,     HTML_SERVER+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'C',     'C',                HTML_SERVER+3,     HTML_SERVER+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'T',     'T',                HTML_SERVER+4,     HTML_SERVER+4,    FALSE, FALSE, NULL, NULL},
/* +04 */   { 'I',     'I',                HTML_SERVER+5,     HTML_SERVER+5,    FALSE, FALSE, NULL, NULL},
/* +05 */   { 'O',     'O',                HTML_SERVER+6,     HTML_SERVER+6,    FALSE, FALSE, NULL, NULL},
/* +06 */   { 'N',     'N',                HTML_SERVER+7,     HTML_SERVER+7,    FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_SERVER+8,     HTML_SERVER+8,    TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_SERVER+9,     HTML_SERVER+12,   TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { '.',     '.',                HTML_SERVER+43,    HTML_SERVER+45,   TRUE , FALSE, WPSHtmlLibName, NULL},
/* +10 */   { '`',     '`',                HTML_SERVER+19,    HTML_SERVER+20,   TRUE , FALSE, WPSHtmlName25, NULL},
/* +11 */   { '?',     '?',                HTML_SERVER+13,    HTML_SERVER+15,   TRUE , FALSE, WPSHtmlName25, NULL},
/* +12 */   { (char)0, (unsigned char)255, HTML_SERVER+9,     HTML_SERVER+12,   TRUE , FALSE, NULL, NULL},
/* +13 */   { '=',     '=',                HTML_SERVER+16,    HTML_SERVER+18,   TRUE , FALSE, WPSHtmlheadersName, NULL},
/* +14 */   { '`',     '`',                HTML_SERVER+19,    HTML_SERVER+20,   TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +15 */   { (char)0, (unsigned char)255, HTML_SERVER+13,    HTML_SERVER+15,   TRUE , FALSE, NULL, NULL},
/* +16 */   { '&',     '&',                HTML_SERVER+13,    HTML_SERVER+15,   TRUE , FALSE, WPSHtmlheadersData, NULL},
/* +17 */   { '`',     '`',                HTML_SERVER+19,    HTML_SERVER+20,   TRUE , FALSE, WPSHtmlheadersData, NULL},
/* +18 */   { (char)0, (unsigned char)255, HTML_SERVER+16,    HTML_SERVER+18,   TRUE , FALSE, NULL, NULL},

/* +19 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlServer, NULL},
/* +20 */   { 'H',     'H',                HTML_SERVER+21,    HTML_SERVER+21,   FALSE, TRUE , NULL, NULL},
/* +21 */   { 'T',     'T',                HTML_SERVER+22,    HTML_SERVER+22,   FALSE, TRUE , NULL, NULL},
/* +22 */   { 'M',     'M',                HTML_SERVER+23,    HTML_SERVER+23,   FALSE, TRUE , NULL, NULL},
/* +23 */   { 'L',     'L',                HTML_SERVER+24,    HTML_SERVER+28,   FALSE, FALSE, NULL, NULL},
/* +24 */   { '=',     '=',                HTML_SERVER+25,    HTML_SERVER+25,   TRUE , FALSE, NULL, NULL},
/* +25 */   { '`',     '`',                HTML_SERVER+26,    HTML_SERVER+28,   TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +26 */   { '`',     '`',                HTML_SERVER+37,    HTML_SERVER+42,   TRUE , TRUE , WPSHtmlHtmlText, NULL},
/* +27 */   { ':',     ':',                HTML_SERVER+29,    HTML_SERVER+36,   TRUE , TRUE , WPSHtmlHtmlText, NULL},
/* +28 */   { (char)0, (unsigned char)255, HTML_SERVER+26,    HTML_SERVER+28,   TRUE , FALSE, NULL, NULL},

/* +29 */   { ':',     ':',                HTML_SERVER+26,    HTML_SERVER+28,   TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +30 */   { '_',     '_',                HTML_SERVER+30,    HTML_SERVER+36,   TRUE , TRUE , NULL, NULL},
/* +31 */   { '#',     '#',                HTML_SERVER+30,    HTML_SERVER+36,   TRUE , TRUE , NULL, NULL},
/* +32 */   { 'A',     'Z',                HTML_SERVER+30,    HTML_SERVER+36,   FALSE, TRUE , NULL, NULL},
/* +33 */   { '0',     '9',                HTML_SERVER+30,    HTML_SERVER+36,   TRUE , TRUE , NULL, NULL},
/* +34 */   { '`',     '`',                HTML_SERVER+37,    HTML_SERVER+42,   TRUE , FALSE, WPSHtmlHtmlVar, NULL},
/* +35 */   { ':',     ':',                HTML_SERVER+30,    HTML_SERVER+36,   TRUE , TRUE , WPSHtmlHtmlVar, NULL},
/* +36 */   { (char)0, (unsigned char)255, HTML_SERVER+26,    HTML_SERVER+28,   TRUE , FALSE, WPSHtmlHtmlVar, NULL},

/* +37 */   { '`',     '`',                HTML_SERVER+26,    HTML_SERVER+28,   TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +38 */   { ' ',     ' ',                HTML_SERVER+42,    HTML_SERVER+42,   TRUE , FALSE, NULL, NULL},
/* +39 */   { '\n',    '\n',               HTML_SERVER+42,    HTML_SERVER+42,   TRUE , FALSE, NULL, NULL},
/* +40 */   { '\r',    '\r',               HTML_SERVER+42,    HTML_SERVER+42,   TRUE , FALSE, NULL, NULL},
/* +41 */   { '\t',    '\t',               HTML_SERVER+42,    HTML_SERVER+42,   TRUE , FALSE, NULL, NULL},
/* +42 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlServer, NULL},

/* +43 */   { '`',     '`',                HTML_SERVER+19,    HTML_SERVER+20,   TRUE , FALSE, WPSHtmlName25, NULL},
/* +44 */   { '?',     '?',                HTML_SERVER+13,    HTML_SERVER+15,   TRUE , FALSE, WPSHtmlName25, NULL},
/* +45 */   { (char)0, (unsigned char)255, HTML_SERVER+43,    HTML_SERVER+45,   TRUE , FALSE, NULL, NULL},

/*************************** HTML_REPEAT ***************************/
/* +00 */   { 'O',     'O',                HTML_ROLLBACK,     HTML_ROLLBACK+1,  FALSE, TRUE , NULL, NULL},
/* +01 */   { 'E',     'E',                HTML_REPEAT+2,     HTML_REPEAT+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'P',     'P',                HTML_REPEAT+3,     HTML_REPEAT+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'E',     'E',                HTML_REPEAT+4,     HTML_REPEAT+4,    FALSE, TRUE , NULL, NULL},
/* +04 */   { 'A',     'A',                HTML_REPEAT+5,     HTML_REPEAT+5,    FALSE, TRUE , NULL, NULL},
/* +05 */   { 'T',     'T',                HTML_ICE_PARAM,    HTML_TYPE-1,      FALSE, FALSE, WPSHtmlRepeat, NULL},

/*************************** HTML_IF ***************************/
/* +00 */   { '(',     '(',                HTML_IF,           HTML_IF+3,        TRUE , FALSE, WPSHtmlIfOpen, NULL},
/* +01 */   { '`',     '`',                HTML_IF+4,         HTML_IF+10,       TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +02 */   { 'T',     'T',                HTML_IF_THEN,      HTML_IF_THEN,     FALSE, FALSE, WPSHtmlIfCond, NULL},
/* +03 */   { 'D',     'D',                HTML_IF_DEF,       HTML_IF_DEF,      FALSE, FALSE, NULL, NULL},

/* +04 */   { '_',     '_',                HTML_IF+4,         HTML_IF+10,       TRUE , TRUE , NULL, NULL},
/* +05 */   { '#',     '#',                HTML_IF+4,         HTML_IF+10,       TRUE , TRUE , NULL, NULL},
/* +06 */   { 'A',     'Z',                HTML_IF+4,         HTML_IF+10,       FALSE, TRUE , NULL, NULL},
/* +07 */   { '0',     '9',                HTML_IF+4,         HTML_IF+10,       TRUE , TRUE , NULL, NULL},
/* +08 */   { '`',     '`',                HTML_IF+11,        HTML_IF+19,       TRUE , TRUE , WPSHtmlIfItem, NULL},
/* +09 */   { ':',     ':',                HTML_IF+4,         HTML_IF+10,       TRUE , TRUE , WPSHtmlIfItem, NULL},
/* +10 */   { (char)0, (unsigned char)255, HTML_IF+8,         HTML_IF+10,       TRUE , FALSE, WPSHtmlIfItem, NULL},

/* +11 */   { '`',     '`',                HTML_IF+8,         HTML_IF+10,       TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +12 */   { ' ',     ' ',                HTML_IF+16,        HTML_IF+19,       TRUE , FALSE, NULL, NULL},
/* +13 */   { '\n',    '\n',               HTML_IF+16,        HTML_IF+19,       TRUE , FALSE, NULL, NULL},
/* +14 */   { '\r',    '\r',               HTML_IF+16,        HTML_IF+19,       TRUE , FALSE, NULL, NULL},
/* +15 */   { '\t',    '\t',               HTML_IF+16,        HTML_IF+19,       TRUE , FALSE, NULL, NULL},
/* +16 */   { '!',     '!',                HTML_IF+20,        HTML_IF+20,       TRUE , TRUE , NULL, NULL},
/* +17 */   { '=',     '=',                HTML_IF+21,        HTML_IF+21,       TRUE , TRUE , NULL, NULL},
/* +18 */   { '>',     '>',                HTML_IF+22,        HTML_IF+22,       TRUE , FALSE, WPSHtmlIfGre, NULL},
/* +19 */   { '<',     '<',                HTML_IF+22,        HTML_IF+22,       TRUE , FALSE, WPSHtmlIfLow, NULL},
/* +20 */   { '=',     '=',                HTML_IF+22,        HTML_IF+22,       TRUE , FALSE, WPSHtmlIfNEq, NULL},
/* +21 */   { '=',     '=',                HTML_IF+22,        HTML_IF+22,       TRUE , FALSE, WPSHtmlIfEq, NULL},
/* +22 */   { '`',     '`',                HTML_IF+23,        HTML_IF+29,       TRUE , TRUE , WPSHtmlMoveAfter, NULL},

/* +23 */   { '_',     '_',                HTML_IF+23,        HTML_IF+29,       TRUE , TRUE , NULL, NULL},
/* +24 */   { '#',     '#',                HTML_IF+23,        HTML_IF+29,       TRUE , TRUE , NULL, NULL},
/* +25 */   { 'A',     'Z',                HTML_IF+23,        HTML_IF+29,       FALSE, TRUE , NULL, NULL},
/* +26 */   { '0',     '9',                HTML_IF+23,        HTML_IF+29,       TRUE , TRUE , NULL, NULL},
/* +27 */   { '`',     '`',                HTML_IF+30,        HTML_IF+38,       TRUE , FALSE, WPSHtmlIfItem, NULL},
/* +28 */   { ':',     ':',                HTML_IF+23,        HTML_IF+29,       TRUE , TRUE , WPSHtmlIfItem, NULL},
/* +29 */   { (char)0, (unsigned char)255, HTML_IF+27,        HTML_IF+29,       TRUE , FALSE, WPSHtmlIfItem, NULL},

/* +30 */   { '`',     '`',                HTML_IF+27,        HTML_IF+29,       TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +31 */   { ' ',     ' ',                HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, NULL, NULL},
/* +32 */   { '\n',    '\n',               HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, NULL, NULL},
/* +33 */   { '\r',    '\r',               HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, NULL, NULL},
/* +34 */   { '\t',    '\t',               HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, NULL, NULL},
/* +35 */   { ')',     ')',                HTML_IF+36,        HTML_IF+39,       TRUE , FALSE, WPSHtmlIfClose, NULL},
/* +36 */   { 'A',     'A',                HTML_IF+40,        HTML_IF+40,       FALSE, FALSE, NULL, NULL},
/* +37 */   { 'O',     'O',                HTML_IF+42,        HTML_IF+42,       FALSE, FALSE, NULL, NULL},
/* +38 */   { 'T',     'T',                HTML_IF_THEN,      HTML_IF_THEN,     FALSE, FALSE, WPSHtmlIfCond, NULL},
/* +39 */   { ')',     ')',                HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, WPSHtmlIfClose, NULL},
/* +40 */   { 'N',     'N',                HTML_IF+41,        HTML_IF+41,       FALSE, FALSE, NULL, NULL},
/* +41 */   { 'D',     'D',                HTML_IF,           HTML_IF+3,        FALSE, FALSE, WPSHtmlIfAnd, NULL},
/* +42 */   { 'R',     'R',                HTML_IF,           HTML_IF+3,        FALSE, FALSE, WPSHtmlIfOr, NULL},

/*************************** HTML_IF_THEN ***************************/
/* +00 */   { 'H',     'H',                HTML_IF_THEN+1,    HTML_IF_THEN+1,   FALSE, TRUE , NULL, NULL},
/* +01 */   { 'E',     'E',                HTML_IF_THEN+2,    HTML_IF_THEN+2,   FALSE, TRUE , NULL, NULL},
/* +02 */   { 'N',     'N',                HTML_IF_THEN+3,    HTML_IF_THEN+3,   FALSE, FALSE, NULL, NULL},
/* +03 */   { '=',     '=',                HTML_IF_THEN+4,    HTML_IF_THEN+4,   TRUE , FALSE, NULL, NULL},
/* +04 */   { '`',     '`',                HTML_IF_THEN+5,    HTML_IF_THEN+7,   TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +05 */   { '`',     '`',                HTML_IF_THEN+16,   HTML_IF_ELSE+1,   TRUE , TRUE , WPSHtmlIfThen, NULL},
/* +06 */   { ':',     ':',                HTML_IF_THEN+8,    HTML_IF_THEN+15,  TRUE , TRUE , WPSHtmlIfThen, NULL},
/* +07 */   { (char)0, (unsigned char)255, HTML_IF_THEN+5,    HTML_IF_THEN+7,   TRUE , FALSE, NULL, NULL},
/* +08 */   { ':',     ':',                HTML_IF_THEN+5,    HTML_IF_THEN+7,   TRUE , TRUE , WPSHtmlIfThen, NULL},
/* +09 */   { '_',     '_',                HTML_IF_THEN+9,    HTML_IF_THEN+15,  TRUE , TRUE , NULL, NULL},
/* +10 */   { '#',     '#',                HTML_IF_THEN+9,    HTML_IF_THEN+15,  TRUE , TRUE , NULL, NULL},
/* +11 */   { 'A',     'Z',                HTML_IF_THEN+9,    HTML_IF_THEN+15,  FALSE, TRUE , NULL, NULL},
/* +12 */   { '0',     '9',                HTML_IF_THEN+9,    HTML_IF_THEN+15,  TRUE , TRUE , NULL, NULL},
/* +13 */   { '`',     '`',                HTML_IF_THEN+16,   HTML_IF_ELSE+1,   TRUE , FALSE, WPSHtmlIfThen, NULL},
/* +14 */   { ':',     ':',                HTML_IF_THEN+9,    HTML_IF_THEN+15,  TRUE , TRUE , WPSHtmlIfThen, NULL},
/* +15 */   { (char)0, (unsigned char)255, HTML_IF_THEN+5,    HTML_IF_THEN+7,   TRUE , FALSE, WPSHtmlIfThen, NULL},
/* +16 */   { '`',     '`',                HTML_IF_THEN+5,    HTML_IF_THEN+7,   TRUE , TRUE , WPSHtmlMoveTo, NULL},
/* +17 */   { ' ',     ' ',                HTML_IF_ELSE,      HTML_IF_ELSE+1,   TRUE , FALSE, NULL, NULL},
/* +18 */   { '\n',    '\n',               HTML_IF_ELSE,      HTML_IF_ELSE+1,   TRUE , FALSE, NULL, NULL},
/* +19 */   { '\r',    '\r',               HTML_IF_ELSE,      HTML_IF_ELSE+1,   TRUE , FALSE, NULL, NULL},
/* +20 */   { '\t',    '\t',               HTML_IF_ELSE,      HTML_IF_ELSE+1,   TRUE , FALSE, NULL, NULL},

/*************************** HTML_IF_ELSE ***************************/
/* +00 */   { '-',     '-',                HTML_END,          HTML_END, TRUE ,  TRUE , NULL , NULL},
/* +01 */   { 'E',     'E',                HTML_IF_ELSE+2,    HTML_IF_ELSE+2,   FALSE, TRUE , NULL, NULL},
/* +02 */   { 'L',     'L',                HTML_IF_ELSE+3,    HTML_IF_ELSE+3,   FALSE, TRUE , NULL, NULL},
/* +03 */   { 'S',     'S',                HTML_IF_ELSE+4,    HTML_IF_ELSE+4,   FALSE, TRUE , NULL, NULL},
/* +04 */   { 'E',     'E',                HTML_IF_ELSE+5,    HTML_IF_ELSE+5,   FALSE, FALSE, NULL, NULL},
/* +05 */   { '=',     '=',                HTML_IF_ELSE+6,    HTML_IF_ELSE+6,   TRUE , FALSE, NULL, NULL},
/* +06 */   { '`',     '`',                HTML_IF_ELSE+7,    HTML_IF_ELSE+9,   TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +07 */   { '`',     '`',                HTML_IF_ELSE+18,   HTML_IF_ELSE+23,  TRUE , TRUE , WPSHtmlIfElse, NULL},
/* +08 */   { ':',     ':',                HTML_IF_ELSE+10,   HTML_IF_ELSE+17,  TRUE , TRUE , WPSHtmlIfElse, NULL},
/* +09 */   { (char)0, (unsigned char)255, HTML_IF_ELSE+7,    HTML_IF_ELSE+9,   TRUE , FALSE, NULL, NULL},
/* +10 */   { ':',     ':',                HTML_IF_ELSE+7,    HTML_IF_ELSE+9,   TRUE , TRUE , WPSHtmlIfElse, NULL},
/* +11 */   { '_',     '_',                HTML_IF_ELSE+11,   HTML_IF_ELSE+17,  TRUE , TRUE , NULL, NULL},
/* +22 */   { '#',     '#',                HTML_IF_ELSE+11,   HTML_IF_ELSE+17,  TRUE , TRUE , NULL, NULL},
/* +13 */   { 'A',     'Z',                HTML_IF_ELSE+11,   HTML_IF_ELSE+17,  FALSE, TRUE , NULL, NULL},
/* +14 */   { '0',     '9',                HTML_IF_ELSE+11,   HTML_IF_ELSE+17,  TRUE , TRUE , NULL, NULL},
/* +15 */   { '`',     '`',                HTML_IF_ELSE+18,   HTML_IF_ELSE+23,  TRUE , TRUE , WPSHtmlIfElse, NULL},
/* +16 */   { ':',     ':',                HTML_IF_ELSE+11,   HTML_IF_ELSE+17,  TRUE , TRUE , WPSHtmlIfElse, NULL},
/* +17 */   { (char)0, (unsigned char)255, HTML_IF_ELSE+7,    HTML_IF_ELSE+9,   TRUE , FALSE, WPSHtmlIfElse, NULL},
/* +18 */   { '`',     '`',                HTML_IF_ELSE+7,    HTML_IF_ELSE+9,   TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +19 */   { ' ',     ' ',                HTML_IF_ELSE+23,   HTML_IF_ELSE+23,  TRUE , FALSE, NULL, NULL},
/* +20 */   { '\n',    '\n',               HTML_IF_ELSE+23,   HTML_IF_ELSE+23,  TRUE , FALSE, NULL, NULL},
/* +21 */   { '\r',    '\r',               HTML_IF_ELSE+23,   HTML_IF_ELSE+23,  TRUE , FALSE, NULL, NULL},
/* +22 */   { '\t',    '\t',               HTML_IF_ELSE+23,   HTML_IF_ELSE+23,  TRUE , FALSE, NULL, NULL},
/* +23 */   { '-',     '-',                HTML_END,          HTML_END, TRUE ,  TRUE , NULL , NULL},

/*************************** HTML_IF_DEF ***************************/
/* +00 */   { 'E',     'E',                HTML_IF_DEF+1,     HTML_IF_DEF+1,    FALSE, TRUE , NULL, NULL},
/* +01 */   { 'F',     'F',                HTML_IF_DEF+2,     HTML_IF_DEF+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'I',     'I',                HTML_IF_DEF+3,     HTML_IF_DEF+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'N',     'N',                HTML_IF_DEF+4,     HTML_IF_DEF+4,    FALSE, TRUE , NULL, NULL},
/* +04 */   { 'E',     'E',                HTML_IF_DEF+5,     HTML_IF_DEF+5,    FALSE, TRUE , NULL, NULL},
/* +05 */   { 'D',     'D',                HTML_IF_DEF+6,     HTML_IF_DEF+6,    FALSE, FALSE, NULL, NULL},
/* +06 */   { '(',     '(',                HTML_IF_DEF+7,     HTML_IF_DEF+8,    TRUE , TRUE , WPSHtmlMoveAfter, NULL},
/* +07 */   { ')',     ')',                HTML_IF+35,        HTML_IF+38,       TRUE , FALSE, WPSHtmlIfDef, NULL},
/* +08 */   { (char)0, (unsigned char)255, HTML_IF_DEF+7,     HTML_IF_DEF+8,    TRUE , TRUE , NULL, NULL},

/*************************** HTML_TRANS ***************************/
/* +00 */   { 'A',     'A',                HTML_TRANS+1,      HTML_TRANS+1,     FALSE, TRUE , NULL, NULL},
/* +01 */   { 'N',     'N',                HTML_TRANS+2,      HTML_TRANS+2,     FALSE, TRUE , NULL, NULL},
/* +02 */   { 'S',     'S',                HTML_TRANS+3,      HTML_TRANS+3,     FALSE, TRUE , NULL, NULL},
/* +03 */   { 'A',     'A',                HTML_TRANS+4,      HTML_TRANS+4,     FALSE, TRUE , NULL, NULL},
/* +04 */   { 'C',     'C',                HTML_TRANS+5,      HTML_TRANS+5,     FALSE, TRUE , NULL, NULL},
/* +05 */   { 'T',     'T',                HTML_TRANS+6,      HTML_TRANS+6,     FALSE, TRUE , NULL, NULL},
/* +06 */   { 'I',     'I',                HTML_TRANS+7,      HTML_TRANS+7,     FALSE, TRUE , NULL, NULL},
/* +07 */   { 'O',     'O',                HTML_TRANS+8,      HTML_TRANS+8,     FALSE, TRUE , NULL, NULL},
/* +08 */   { 'N',     'N',                HTML_TRANS+9,      HTML_TRANS+9,     FALSE, FALSE, NULL, NULL},
/* +09 */   { '=',     '=',                HTML_TRANS+10,     HTML_TRANS+10,    TRUE , FALSE, NULL, NULL},
/* +10 */   { '`',     '`',                HTML_TRANS+11,     HTML_TRANS+12,    TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +11 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlTransaction, NULL},
/* +12 */   { (char)0, (unsigned char)255, HTML_TRANS+11,     HTML_TRANS+12,    TRUE , FALSE, NULL, NULL},

/*************************** HTML_COMMIT ***************************/
/* +00 */   { 'U',     'U',                HTML_CURSOR,       HTML_CURSOR,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'O',     'O',                HTML_COMMIT+2,     HTML_COMMIT+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'M',     'M',                HTML_COMMIT+3,     HTML_COMMIT+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'M',     'M',                HTML_COMMIT+4,     HTML_COMMIT+4,    FALSE, TRUE , NULL, NULL},
/* +04 */   { 'I',     'I',                HTML_COMMIT+5,     HTML_COMMIT+5,    FALSE, TRUE , NULL, NULL},
/* +05 */   { 'T',     'T',                HTML_COMMIT+6,     HTML_COMMIT+6,    FALSE, FALSE, NULL, NULL},
/* +06 */   { '=',     '=',                HTML_COMMIT+7,     HTML_COMMIT+7,    TRUE , FALSE, NULL, NULL},
/* +07 */   { '`',     '`',                HTML_COMMIT+8,     HTML_COMMIT+9,    TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +08 */   { '`',     '`',                HTML_COMMIT+10,    HTML_COMMIT+10,   TRUE , FALSE, WPSHtmlTransaction, NULL},
/* +09 */   { (char)0, (unsigned char)255, HTML_COMMIT+8,     HTML_COMMIT+9,    TRUE , FALSE, NULL, NULL},
/* +10 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlCommit, NULL},

/*************************** HTML_ROLLBACK ***************************/
/* +00 */   { 'W',     'W',                HTML_ROWS,         HTML_ROWS,        FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_ROLLBACK+2,   HTML_ROLLBACK+2,  FALSE, TRUE , NULL, NULL},
/* +02 */   { 'L',     'L',                HTML_ROLLBACK+3,   HTML_ROLLBACK+3,  FALSE, TRUE , NULL, NULL},
/* +03 */   { 'B',     'B',                HTML_ROLLBACK+4,   HTML_ROLLBACK+4,  FALSE, TRUE , NULL, NULL},
/* +04 */   { 'A',     'A',                HTML_ROLLBACK+5,   HTML_ROLLBACK+5,  FALSE, TRUE , NULL, NULL},
/* +05 */   { 'C',     'C',                HTML_ROLLBACK+6,   HTML_ROLLBACK+6,  FALSE, TRUE , NULL, NULL},
/* +06 */   { 'K',     'K',                HTML_ROLLBACK+7,   HTML_ROLLBACK+7,  FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_ROLLBACK+8,   HTML_ROLLBACK+8,  TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_ROLLBACK+9,   HTML_ROLLBACK+10, TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { '`',     '`',                HTML_ROLLBACK+11,  HTML_ROLLBACK+11, TRUE , FALSE, WPSHtmlTransaction, NULL},
/* +10 */   { (char)0, (unsigned char)255, HTML_ROLLBACK+9,   HTML_ROLLBACK+10, TRUE , FALSE, NULL, NULL},
/* +11 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , WPSHtmlRollback, NULL},

/*************************** HTML_CURSOR ***************************/
/* +00 */   { 'R',     'R',                HTML_CURSOR+1,     HTML_CURSOR+1,    FALSE, TRUE , NULL, NULL},
/* +01 */   { 'S',     'S',                HTML_CURSOR+2,     HTML_CURSOR+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'O',     'O',                HTML_CURSOR+3,     HTML_CURSOR+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'R',     'R',                HTML_CURSOR+4,     HTML_CURSOR+4,    FALSE, FALSE, NULL, NULL},
/* +04 */   { '=',     '=',                HTML_CURSOR+5,     HTML_CURSOR+5,    TRUE , FALSE, NULL, NULL},
/* +05 */   { '`',     '`',                HTML_CURSOR+6,     HTML_CURSOR+7,    TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +06 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlCursor, NULL},
/* +07 */   { (char)0, (unsigned char)255, HTML_CURSOR+6,     HTML_CURSOR+7,    TRUE , FALSE, NULL, NULL},

/*************************** HTML_ROWS ***************************/
/* +00 */   { 'S',     'S',                HTML_ROWS+1,       HTML_ROWS+1,      FALSE, FALSE , NULL, NULL},
/* +01 */   { '=',     '=',                HTML_ROWS+2,       HTML_ROWS+2,      TRUE , FALSE, NULL, NULL},
/* +02 */   { '`',     '`',                HTML_ROWS+3,       HTML_ROWS+4,      TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +03 */   { '`',     '`',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, WPSHtmlRows, NULL},
/* +04 */   { (char)0, (unsigned char)255, HTML_ROWS+3,       HTML_ROWS+4,      TRUE , FALSE, NULL, NULL},

/*************************** HTML_SWITCH ***************************/
/* +00 */   { 'I',     'I',                HTML_SWITCH+1,     HTML_SWITCH+1,    FALSE, TRUE , NULL, NULL},
/* +01 */   { 'T',     'T',                HTML_SWITCH+2,     HTML_SWITCH+2,    FALSE, TRUE , NULL, NULL},
/* +02 */   { 'C',     'C',                HTML_SWITCH+3,     HTML_SWITCH+3,    FALSE, TRUE , NULL, NULL},
/* +03 */   { 'H',     'H',                HTML_SWITCH+4,     HTML_SWITCH+4,    FALSE, FALSE, NULL, NULL},
/* +04 */   { '=',     '=',                HTML_SWITCH+5,     HTML_SWITCH+5,    TRUE , FALSE, NULL, NULL},
/* +05 */   { '`',     '`',                HTML_SWITCH+6,     HTML_SWITCH+8,    TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +06 */   { '`',     '`',                HTML_SWITCH+17,    HTML_CASE,        TRUE , TRUE , WPSHtmlSWSrc, NULL},
/* +07 */   { ':',     ':',                HTML_SWITCH+9,     HTML_SWITCH+16,   TRUE , TRUE , WPSHtmlSWSrc, NULL},
/* +08 */   { (char)0, (unsigned char)255, HTML_SWITCH+6,     HTML_SWITCH+8,    TRUE , FALSE, NULL, NULL},
/* +09 */   { ':',     ':',                HTML_SWITCH+6,     HTML_SWITCH+8,    TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +10 */   { '_',     '_',                HTML_SWITCH+10,    HTML_SWITCH+16,   FALSE, TRUE , NULL, NULL},
/* +11 */   { '#',     '#',                HTML_SWITCH+10,    HTML_SWITCH+16,   FALSE, TRUE , NULL, NULL},
/* +12 */   { 'A',     'Z',                HTML_SWITCH+10,    HTML_SWITCH+16,   FALSE, TRUE , NULL, NULL},
/* +13 */   { '0',     '9',                HTML_SWITCH+10,    HTML_SWITCH+16,   TRUE , TRUE , NULL, NULL},
/* +14 */   { '`',     '`',                HTML_SWITCH+17,    HTML_CASE,        TRUE , TRUE , WPSHtmlSWSrc, NULL},
/* +15 */   { ':',     ':',                HTML_SWITCH+9,     HTML_SWITCH+16,   TRUE , TRUE , WPSHtmlSWSrc, NULL},
/* +16 */   { (char)0, (unsigned char)255, HTML_SWITCH+6,     HTML_SWITCH+8,    TRUE , FALSE, WPSHtmlSWSrc, NULL},
/* +17 */   { '`',     '`',                HTML_SWITCH+6,     HTML_SWITCH+8,    TRUE , TRUE , WPSHtmlMoveTo, NULL},
/* +18 */   { ' ',     ' ',                HTML_SWITCH+22,    HTML_CASE,        TRUE , FALSE, NULL, NULL},
/* +19 */   { '\n',    '\n',               HTML_SWITCH+22,    HTML_CASE,        TRUE , FALSE, NULL, NULL},
/* +20 */   { '\r',    '\r',               HTML_SWITCH+22,    HTML_CASE,        TRUE , FALSE, NULL, NULL},
/* +21 */   { '\t',    '\t',               HTML_SWITCH+22,    HTML_CASE,        TRUE , FALSE, NULL, NULL},
/* +22 */   { 'D',     'D',                HTML_DEFAULT+1,    HTML_DEFAULT+1,   FALSE, TRUE , NULL, NULL},

/*************************** HTML_CASE ***************************/
/* +00 */   { 'C',     'C',                HTML_CASE+1,       HTML_CASE+1,      FALSE, TRUE , NULL, NULL},
/* +01 */   { 'A',     'A',                HTML_CASE+2,       HTML_CASE+2,      FALSE, TRUE , NULL, NULL},
/* +02 */   { 'S',     'S',                HTML_CASE+3,       HTML_CASE+3,      FALSE, TRUE , NULL, NULL},
/* +03 */   { 'E',     'E',                HTML_CASE+4,       HTML_CASE+4,      FALSE, FALSE, NULL, NULL},
/* +04 */   { '`',     '`',                HTML_CASE+5,       HTML_CASE+7,      TRUE , FALSE, WPSHtmlSWNext, NULL},
/* +05 */   { '`',     '`',                HTML_CASE+16,      HTML_CASE+21,     TRUE , TRUE , WPSHtmlValue25, NULL},
/* +06 */   { ':',     ':',                HTML_CASE+8,       HTML_CASE+15,     TRUE , TRUE , WPSHtmlValue25, NULL},
/* +07 */   { (char)0, (unsigned char)255, HTML_CASE+5,       HTML_CASE+7,      TRUE , FALSE, NULL, NULL},
/* +08 */   { ':',     ':',                HTML_CASE+5,       HTML_CASE+7,      TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +09 */   { '_',     '_',                HTML_CASE+9,       HTML_CASE+15,     FALSE, TRUE , NULL, NULL},
/* +10 */   { '#',     '#',                HTML_CASE+9,       HTML_CASE+15,     FALSE, TRUE , NULL, NULL},
/* +11 */   { 'A',     'Z',                HTML_CASE+9,       HTML_CASE+15,     FALSE, TRUE , NULL, NULL},
/* +12 */   { '0',     '9',                HTML_CASE+9,       HTML_CASE+15,     TRUE , TRUE , NULL, NULL},
/* +13 */   { '`',     '`',                HTML_CASE+16,      HTML_CASE+21,     TRUE , TRUE , WPSHtmlValue25, NULL},
/* +14 */   { ':',     ':',                HTML_CASE+8,       HTML_CASE+15,     TRUE , TRUE , WPSHtmlValue25, NULL},
/* +15 */   { (char)0, (unsigned char)255, HTML_CASE+5,       HTML_CASE+7,      TRUE , FALSE, WPSHtmlValue25, NULL},
/* +16 */   { '`',     '`',                HTML_CASE+5,       HTML_CASE+7,      TRUE , TRUE , WPSHtmlMoveTo, NULL},
/* +17 */   { ' ',     ' ',                HTML_CASE+21,      HTML_CASE+21,     TRUE , FALSE, NULL, NULL},
/* +18 */   { '\n',    '\n',               HTML_CASE+21,      HTML_CASE+21,     TRUE , FALSE, NULL, NULL},
/* +19 */   { '\r',    '\r',               HTML_CASE+21,      HTML_CASE+21,     TRUE , FALSE, NULL, NULL},
/* +20 */   { '\t',    '\t',               HTML_CASE+21,      HTML_CASE+21,     TRUE , FALSE, NULL, NULL},
/* +21 */   { '=',     '=',                HTML_CASE+22,      HTML_CASE+22,     TRUE , FALSE, NULL, NULL},
/* +22 */   { '`',     '`',                HTML_CASE+23,      HTML_CASE+25,     TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +23 */   { '`',     '`',                HTML_CASE+34,      HTML_DEFAULT,     TRUE , TRUE , WPSHtmlSWCase, NULL},
/* +24 */   { ':',     ':',                HTML_CASE+26,      HTML_CASE+33,     TRUE , TRUE , WPSHtmlSWCase, NULL},
/* +25 */   { (char)0, (unsigned char)255, HTML_CASE+23,      HTML_CASE+25,     TRUE , FALSE, NULL, NULL},
/* +26 */   { ':',     ':',                HTML_CASE+23,      HTML_CASE+25,     TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +27 */   { '_',     '_',                HTML_CASE+27,      HTML_CASE+33,     FALSE, TRUE , NULL, NULL},
/* +28 */   { '#',     '#',                HTML_CASE+27,      HTML_CASE+33,     FALSE, TRUE , NULL, NULL},
/* +29 */   { 'A',     'Z',                HTML_CASE+27,      HTML_CASE+33,     FALSE, TRUE , NULL, NULL},
/* +30 */   { '0',     '9',                HTML_CASE+27,      HTML_CASE+33,     TRUE , TRUE , NULL, NULL},
/* +31 */   { '`',     '`',                HTML_CASE+34,      HTML_DEFAULT,     TRUE , TRUE , WPSHtmlSWCase, NULL},
/* +32 */   { ':',     ':',                HTML_CASE+26,      HTML_CASE+33,     TRUE , TRUE , WPSHtmlSWCase, NULL},
/* +33 */   { (char)0, (unsigned char)255, HTML_CASE+23,      HTML_CASE+25,     TRUE , FALSE, WPSHtmlSWCase, NULL},
/* +34 */   { '`',     '`',                HTML_CASE+23,      HTML_CASE+25,     TRUE , TRUE , WPSHtmlMoveTo, NULL},
/* +35 */   { ' ',     ' ',                HTML_CASE+39,      HTML_DEFAULT,     TRUE , FALSE, NULL, NULL},
/* +36 */   { '\n',    '\n',               HTML_CASE+39,      HTML_DEFAULT,     TRUE , FALSE, NULL, NULL},
/* +37 */   { '\r',    '\r',               HTML_CASE+39,      HTML_DEFAULT,     TRUE , FALSE, NULL, NULL},
/* +38 */   { '\t',    '\t',               HTML_CASE+39,      HTML_DEFAULT,     TRUE , FALSE, NULL, NULL},
/* +39 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , NULL, NULL},
/* +40 */   { 'C',     'C',                HTML_CASE+1,       HTML_CASE+1,      FALSE, TRUE , NULL, NULL},

/*************************** HTML_DEFAULT ***************************/
/* +00 */   { 'D',     'D',                HTML_DEFAULT+1,    HTML_DEFAULT+1,   FALSE, TRUE , NULL, NULL},
/* +01 */   { 'E',     'E',                HTML_DEFAULT+2,    HTML_DEFAULT+2,   FALSE, TRUE , NULL, NULL},
/* +02 */   { 'F',     'F',                HTML_DEFAULT+3,    HTML_DEFAULT+3,   FALSE, TRUE , NULL, NULL},
/* +03 */   { 'A',     'A',                HTML_DEFAULT+4,    HTML_DEFAULT+4,   FALSE, TRUE , NULL, NULL},
/* +04 */   { 'U',     'U',                HTML_DEFAULT+5,    HTML_DEFAULT+5,   FALSE, TRUE , NULL, NULL},
/* +05 */   { 'L',     'L',                HTML_DEFAULT+6,    HTML_DEFAULT+6,   FALSE, TRUE , NULL, NULL},
/* +06 */   { 'T',     'T',                HTML_DEFAULT+7,    HTML_DEFAULT+7,   FALSE, FALSE, NULL, NULL},
/* +07 */   { '=',     '=',                HTML_DEFAULT+8,    HTML_DEFAULT+8,   TRUE , FALSE, NULL, NULL},
/* +08 */   { '`',     '`',                HTML_DEFAULT+9,    HTML_DEFAULT+11,  TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +09 */   { '`',     '`',                HTML_DEFAULT+20,   HTML_DEFAULT+25,  TRUE , TRUE , WPSHtmlSWDefault, NULL},
/* +10 */   { ':',     ':',                HTML_DEFAULT+12,   HTML_DEFAULT+19,  TRUE , TRUE , WPSHtmlSWDefault, NULL},
/* +11 */   { (char)0, (unsigned char)255, HTML_DEFAULT+9,    HTML_DEFAULT+11,  TRUE , FALSE, NULL, NULL},
/* +12 */   { ':',     ':',                HTML_DEFAULT+9,    HTML_DEFAULT+11,  TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +13 */   { '_',     '_',                HTML_DEFAULT+13,   HTML_DEFAULT+19,  FALSE, TRUE , NULL, NULL},
/* +14 */   { '#',     '#',                HTML_DEFAULT+13,   HTML_DEFAULT+19,  FALSE, TRUE , NULL, NULL},
/* +15 */   { 'A',     'Z',                HTML_DEFAULT+13,   HTML_DEFAULT+19,  FALSE, TRUE , NULL, NULL},
/* +16 */   { '0',     '9',                HTML_DEFAULT+13,   HTML_DEFAULT+19,  TRUE , TRUE , NULL, NULL},
/* +17 */   { '`',     '`',                HTML_DEFAULT+20,   HTML_DEFAULT+25,  TRUE , TRUE , WPSHtmlSWDefault, NULL},
/* +18 */   { ':',     ':',                HTML_DEFAULT+12,   HTML_DEFAULT+19,  TRUE , TRUE , WPSHtmlSWDefault, NULL},
/* +19 */   { (char)0, (unsigned char)255, HTML_DEFAULT+9,    HTML_DEFAULT+11,  TRUE , FALSE, WPSHtmlSWDefault, NULL},
/* +20 */   { '`',     '`',                HTML_DEFAULT+9,    HTML_DEFAULT+11,  TRUE , TRUE , WPSHtmlMoveTo, NULL},
/* +21 */   { ' ',     ' ',                HTML_DEFAULT+25,   HTML_DEFAULT+25,  TRUE , FALSE, NULL, NULL},
/* +22 */   { '\n',    '\n',               HTML_DEFAULT+25,   HTML_DEFAULT+25,  TRUE , FALSE, NULL, NULL},
/* +23 */   { '\r',    '\r',               HTML_DEFAULT+25,   HTML_DEFAULT+25,  TRUE , FALSE, NULL, NULL},
/* +24 */   { '\t',    '\t',               HTML_DEFAULT+25,   HTML_DEFAULT+25,  TRUE , FALSE, NULL, NULL},
/* +25 */   { '-',     '-',                HTML_END,          HTML_END,         TRUE , TRUE , NULL, NULL},

/*************************** HTML_XML ***************************/
/* +00 */   { 'M',     'M',                HTML_XML+1,        HTML_XML+1,       FALSE, TRUE , NULL, NULL},
/* +01 */   { 'L',     'L',                HTML_XML+2,        HTML_XML+3,       FALSE, FALSE, WPSXmlTypeRaw, NULL},
/* +02 */   { '=',     '=',                HTML_XML+9,        HTML_XML+9,       TRUE , FALSE, NULL, NULL},
/* +03 */   { 'P',     'P',                HTML_XML+4,        HTML_XML+4,       FALSE, TRUE , NULL, NULL},
/* +04 */   { 'D',     'D',                HTML_XML+5,        HTML_XML+5,       FALSE, TRUE , NULL, NULL},
/* +05 */   { 'A',     'A',                HTML_XML+6,        HTML_XML+6,       FALSE, TRUE , NULL, NULL},
/* +06 */   { 'T',     'T',                HTML_XML+7,        HTML_XML+7,       FALSE, TRUE , NULL, NULL},
/* +07 */   { 'A',     'A',                HTML_XML+8,        HTML_XML+8,       FALSE, FALSE, WPSXmlTypePdata, NULL},
/* +08 */   { '=',     '=',                HTML_XML+9,        HTML_XML+9,       TRUE , FALSE, NULL, NULL},
/* +09 */   { '`',     '`',                HTML_XML+10,       HTML_XML+12,       TRUE , FALSE, WPSHtmlMoveAfter, NULL},
/* +10 */   { '`',     '`',                HTML_XML+21,       HTML_XML+25,      TRUE , TRUE , WPSMarkupText, NULL},
/* +11 */   { ':',     ':',                HTML_XML+13,       HTML_XML+20,      TRUE , TRUE , WPSMarkupText, NULL},
/* +12 */   { (char)0, (unsigned char)255, HTML_XML+10,       HTML_XML+12,       TRUE , FALSE, NULL, NULL},
/* +13 */   { ':',     ':',                HTML_XML+10,       HTML_XML+12,       TRUE , TRUE , WPSHtmlDoubleChar, NULL},
/* +14 */   { '_',     '_',                HTML_XML+14,       HTML_XML+20,      TRUE , TRUE , NULL, NULL},
/* +15 */   { '#',     '#',                HTML_XML+14,       HTML_XML+20,      TRUE , TRUE , NULL, NULL},
/* +16 */   { 'A',     'Z',                HTML_XML+14,       HTML_XML+20,      FALSE, TRUE , NULL, NULL},
/* +17 */   { '0',     '9',                HTML_XML+14,       HTML_XML+20,      TRUE , TRUE , NULL, NULL},
/* +18 */   { '`',     '`',                HTML_XML+21,       HTML_XML+25,      TRUE , TRUE , WPSXmlMarkupVar, NULL},
/* +19 */   { ':',     ':',                HTML_XML+14,       HTML_XML+20,      TRUE , TRUE , WPSXmlMarkupVar, NULL},
/* +20 */   { (char)0, (unsigned char)255, HTML_XML+10,       HTML_XML+12,       TRUE , FALSE, WPSXmlMarkupVar, NULL},
/* +21 */   { '`',     '`',                HTML_XML+10,       HTML_XML+12,       TRUE , FALSE, WPSHtmlMoveTo, NULL},
/* +22 */   { ' ',     ' ',                HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +23 */   { '\n',    '\n',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +24 */   { '\r',    '\r',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
/* +25 */   { '\t',    '\t',               HTML_ICE_PARAM,    HTML_TYPE-1,      TRUE , FALSE, NULL, NULL},
};

# define WPS_NUM_PARSER_NODES (sizeof (html) / sizeof (PARSER_NODE))

/*
** Name: WPSNumParserNodes
**
** Description:
**      Returns the number of nodes in the node table.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      Number of nodes.
**      There is no error return.
**
** History:
**      02-Nov-98 (fanra01)
**          Created.
*/
i4
WPSNumParserNodes ()
{
    return (WPS_NUM_PARSER_NODES);
}

/*
** Name: WPSDisplayParserNode
**
** Description:
**      Prints the node entry
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      02-Nov-98 (fanra01)
**          Created.
*/
void
WPSDisplayParserNode (i4 i)
{
    PARSER_NODE*    p = &html[i];

    SIprintf ("%4d '%c'(%02x) - '%c'(%02x) %04d  %04d %5s %5s %08p %08p\n",
        i,
        isprint(p->min) ? p->min : ' ', p->min,
        isprint(p->max) ? p->max : ' ', p->max,
        p->begin, p->end,
        (p->Case ? "FALSE" : "TRUE"),
        (p->Separator ? "FALSE" : "TRUE"),
        p->execution,
        p->endExecution);
    return;
}
