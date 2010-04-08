/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/
/*
**  Project  : CA-OpenIngres/Visual DBA
**
**  Source   : Parse.h, Header File
**
**  Author   : UK Sotheavut.
**  Purpose  : Function prototype for accessing low level data
**
**   History
**   22-Oct-2003 (schph01)
**   (SIR 111153) add GetRmcmdObjects_launcher() function prototype to manage
**   the rmcmd command 'ddprocessuser'
**
*/
#if !defined (DOM_RIGHTPANE_TABLE_STATISTIC_PARSE_HEADER)
#define DOM_RIGHTPANE_TABLE_STATISTIC_PARSE_HEADER
#include "tbstatis.h"

//
// Query all the Table's columns either with statistic or not:
// IN : ts.
// OUT: ts.m_listColumn.
BOOL Table_QueryStatisticColumns (CaTableStatistic& ts);

//
// Query the statistic for a given column <pColumn->m_strColumn>:
// IN : pColumn, ts.
// OUT: ts.m_listItem.
BOOL Table_GetStatistics (CaTableStatisticColumn* pColumn, CaTableStatistic& ts);

//
// Generate the statistic for a given column <pColumn->m_strColumn>:
// IN : pColumn, ts.
// OUT: ?
BOOL Table_GenerateStatistics ( HWND CurHwnd, CaTableStatisticColumn* pColumn, CaTableStatistic& ts);
//
// Remove the statistic for a given column <pColumn->m_strColumn>:
// IN : pColumn, ts.
// OUT: ?
BOOL Table_RemoveStatistics (CaTableStatisticColumn* pColumn, CaTableStatistic& ts);
//
// Get user lauching rmcmd process
// IN : ?
// OUT : user name
BOOL GetRmcmdObjects_launcher ( char *nodeName, char *objName);

#endif

