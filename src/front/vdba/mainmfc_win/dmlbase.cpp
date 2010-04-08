/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : dmlbase.cpp Implementation File                                       //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres                                                         //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Base class for Data manipulation                  .                   //
****************************************************************************************/

#include "stdafx.h"
#include "dmlbase.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Object: General (Item + Owner)
// -----------------------------
IMPLEMENT_SERIAL (CaDBObject, CObject, 1)
void CaDBObject::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strItem;
		ar << m_strItemOwner;
	}
	else
	{
		ar >> m_strItem;
		ar >> m_strItemOwner;
	}
}


//
// Object: Column 
// --------------
IMPLEMENT_SERIAL (CaColumn, CaDBObject, 1)
void CaColumn::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{

	}
	else
	{

	}
}


//
// Object: Key 
// -------------
IMPLEMENT_SERIAL (CaKey, CObject, 1)
void CaKey::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strKeyName;
	}
	else
	{
		ar >> m_strKeyName;
	}
	m_listColumn.Serialize (ar);
}


//
// Object: Foreign Key 
// -------------------
IMPLEMENT_SERIAL (CaForeignKey, CaKey, 1)
void CaForeignKey::Serialize (CArchive& ar)
{
	CaKey::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strReferencedTable;
		ar << m_strReferencedTableOwner;
		ar << m_strReferencedKey;
	}
	else
	{
		ar >> m_strReferencedTable;
		ar >> m_strReferencedTableOwner;
		ar >> m_strReferencedKey;
	}
	m_listReferringColumn.Serialize (ar);
}
