/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xvsdadml.h, header file 
**    Project  : Schema Diff
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Visual SDA.
**
** History:
**
** 24-Sep-2002 (uk$so01)
**    Created
*/

#if !defined (XVSDADML_HEADER)
#define XVSDADML_HEADER
class CaNode;
class CdSda;
class CaDBObject;

BOOL DML_QueryNode(CdSda* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryServer(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryUser(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryDatabase(CdSda* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj);


#endif // XVSDADML_HEADER