/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xipmdml.h, Header file 
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Ingres Performance Monitor.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
*/

#if !defined (XIPMDML_HEADER)
#define XIPMDML_HEADER
#include "vnodedml.h"
class CdSqlQuery;

BOOL DML_QueryNode(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryServer(CdSqlQuery* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryUser(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryDatabase(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj, int& nSessionVersion);
#if defined (_OPTION_GROUPxROLE)
BOOL DML_QueryGroup(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryRole(CdSqlQuery* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
#endif


#endif // XIPMDML_HEADER