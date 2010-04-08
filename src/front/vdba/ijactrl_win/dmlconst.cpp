/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dmlconst.cpp , Implementation File 
**    Project  : IJA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Constraint Object (Primary Key, Unique Key, ...) of Table
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "dmlconst.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Rule
// -------------
IMPLEMENT_SERIAL (CaConstraint, CaDBObject, 1)
void CaConstraint::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strTable;
		ar << m_strText;
		ar << m_strType;
	}
	else
	{
		ar >> m_strTable;
		ar >> m_strText;
		ar >> m_strType;
	}
}





