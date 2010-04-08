/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : parse.h, Header File 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Function prototype for accessing low level data 
**
** History:
**
** 24-Jul-2001 (uk$so01)
**    Created. Split the old code from VDBA
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined (QEPxTRACE_PARSE_HEADER)
#define QEPxTRACE_PARSE_HEADER

//
// pMainDoc    : Contains information the current node, ...
//               pMainDoc->m_strNode
//               pMainDoc->m_strServer
//               pMainDoc->m_strDatabase (Current database that is being connected).
//               ...
// strStatement: The statement to execute.
// bSelect     : TRUE if strStatement is a Select Statement.
// pDoc        : Document that will contain the list of Qep's trees.
class CdQueryExecutionPlanDoc;
class CdSqlQueryRichEditDoc;
BOOL QEPParse (CdSqlQueryRichEditDoc* pMainDoc, LPCTSTR strStatement, BOOL bSelect, CdQueryExecutionPlanDoc* pDoc);




#endif // QEPxTRACE_PARSE_HEADER

