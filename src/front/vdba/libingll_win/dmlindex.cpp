/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlindex.cpp: implementation for the CaIndex class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Index object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 17-Mar-2003 (schph01)
**    SIR #109220 Add data member m_nFlag to manage index in Star Database
**/

#include "stdafx.h"
#include "dmlindex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Index 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIndex, CaDBObject, 1)
CaIndex::CaIndex(const CaIndex& c)
{
	Copy (c);
}

CaIndex CaIndex::operator = (const CaIndex& c)
{
	Copy (c);
	return *this;
}

void CaIndex::Copy (const CaIndex& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_nFlag = c.m_nFlag;
}

CaIndex::~CaIndex()
{
}

void CaIndex::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();
		ar << m_nFlag;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;
		ar >> m_nFlag;
	}
}

BOOL CaIndex::Matched (CaIndex* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||
	    nFlag == MATCHED_NAMExOWNER)
	{
		return CaDBObject::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;
		if (pObj->GetFlag() != m_nFlag)
			return FALSE;
		//
		// TODO:
		ASSERT(FALSE);
		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}


//
// Object: Index Detail 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIndexDetail, CaIndex, 1)
CaIndexDetail::CaIndexDetail(const CaIndexDetail& c)
{
	Copy (c);
}


CaIndexDetail CaIndexDetail::operator =(const CaIndexDetail& c)
{
	Copy (c);
	return *this;
}


CaIndexDetail::~CaIndexDetail()
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
}

void CaIndexDetail::Serialize (CArchive& ar)
{
	CaIndex::Serialize (ar);
	m_storageStructure.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strBaseTable;
		ar << m_strBaseTableOwner;
		ar << m_strCreateDate;
		ar << m_cPersistent;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strBaseTable;
		ar >> m_strBaseTableOwner;
		ar >> m_strCreateDate;
		ar >> m_cPersistent;
	}
	m_listColumn.Serialize(ar);
}


BOOL CaIndexDetail::Matched (CaIndexDetail* pObj, MatchObjectFlag nFlag)
{


	return TRUE;
}


void CaIndexDetail::Copy(const CaIndexDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaIndex::Copy(c);
	// 
	// Extra copy here:
	ASSERT(FALSE); // To be implemented
}

