/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**    Source   : dmlbase.cpp , Implementation File 
**    Project  : Ingres II / Visual DBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Base class of database object
**
** History:
**
** 05-Jan-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

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
CaDBObject::CaDBObject(): CObject(), m_strItem(""), m_strItemOwner(""), m_bSystem (FALSE)
{
	m_tTimeStamp = -1;
	m_nObjectType = -1;
	m_bSerializeAllData = FALSE;
}

CaDBObject::CaDBObject(LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem):CObject()
{
	m_strItem = lpszItem;
	m_strItemOwner = lpszItemOwner;
	m_bSystem   = bSystem;
	m_strItem.TrimRight();
	m_strItemOwner.TrimRight();
	m_tTimeStamp = -1;
	m_nObjectType = -1;
	m_bSerializeAllData = FALSE;
}

void CaDBObject::SetItem (LPCTSTR lpszItem, LPCTSTR lpszItemOwner, BOOL bSystem)
{
	m_strItem = lpszItem;
	m_strItemOwner = lpszItemOwner;
	m_bSystem   = bSystem;
	m_strItem.TrimRight();
	m_strItemOwner.TrimRight();
}

void CaDBObject::Serialize (CArchive& ar)
{
	CObject::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strItem;
		ar << m_strItemOwner;
		ar << m_bSystem;
		ar << m_bSerializeAllData;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strItem;
		ar >> m_strItemOwner;
		ar >> m_bSystem;
		ar >> m_bSerializeAllData;
	}
}


CaDBObject::CaDBObject(const CaDBObject& c)
{
	Copy(c);
}

CaDBObject CaDBObject::operator = (const CaDBObject& c)
{
	Copy(c);
	return *this;
}

void CaDBObject::Copy (const CaDBObject& c)
{
	m_strItem      = c.m_strItem;
	m_strItemOwner = c.m_strItemOwner;
	m_bSystem      = c.m_bSystem;
	m_nObjectType  = c.m_nObjectType;
	m_bSerializeAllData= c.m_bSerializeAllData;
}


BOOL CaDBObject::Matched (CaDBObject* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetName().CompareNoCase (m_strItem) == 0);
	}
	else
	if (nFlag == MATCHED_NAME ||
	    nFlag == MATCHED_NAMExOWNER)
	{
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;
		if (pObj->GetOwner().CompareNoCase (m_strItemOwner) != 0)
			return FALSE;
		return TRUE;
	}

	//
	// Only Name and Owner for the base object:
	ASSERT (FALSE);
	return FALSE;
}


CaDBObject* CaDBObject::FindDBObject(LPCTSTR lpszName, CTypedPtrList < CObList, CaDBObject* >& l)
{
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = l.GetNext(pos);
		if (pObj->GetName().CompareNoCase(lpszName) == 0)
			return pObj;
	}
	return NULL;
}

CaDBObject* CaDBObject::FindDBObject(LPCTSTR lpszName, LPCTSTR lpszOwner, CTypedPtrList < CObList, CaDBObject* >& l)
{
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaDBObject* pObj = l.GetNext(pos);
		if (pObj->GetName().CompareNoCase(lpszName) == 0 && pObj->GetOwner().CompareNoCase(lpszOwner))
			return pObj;
	}
	return NULL;
}
