/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xipmdml.h, Header file 
**    Project  : IPM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Stand alone Ingres Performance Monitor.
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
*/

#if !defined (XIPMDML_HEADER)
#define XIPMDML_HEADER
#include "vnodedml.h"
class CdIpm;

BOOL DML_QueryNode(CdIpm* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryServer(CdIpm* pDoc, CaNode* pNode, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_QueryUser(CdIpm* pDoc, CTypedPtrList< CObList, CaDBObject* >& listObj);
BOOL DML_FillComboResourceType(CComboBox* pCombo);


#endif // XIPMDML_HEADER