/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlgroup.cpp: Implementation for the CaGroup class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Group object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlgroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Group
// ************************************************************************************************
IMPLEMENT_SERIAL (CaGroup, CaDBObject, 1)
CaGroup::CaGroup(const CaGroup& c)
{
	Copy (c);
}

CaGroup CaGroup::operator = (const CaGroup& c)
{
	Copy (c);
	return *this;
}

void CaGroup::Copy (const CaGroup& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
}

void CaGroup::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

	}
}

BOOL CaGroup::Matched (CaGroup* pObj, MatchObjectFlag nFlag)
{
	if (pObj->GetName().CompareNoCase (m_strItem) != 0)
		return FALSE;
	return TRUE;
}



//
// Object: CaGroupDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaGroupDetail, CaGroup, 1)
CaGroupDetail::CaGroupDetail(const CaGroupDetail& c)
{
	Copy(c);
}

CaGroupDetail CaGroupDetail::operator = (const CaGroupDetail& c)
{
	Copy (c);
	return *this;
}

void CaGroupDetail::Serialize (CArchive& ar)
{
	CaGroup::Serialize (ar);
	m_listMember.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

	}
}

BOOL CaGroupDetail::Matched (CaGroupDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaGroup::Matched (pObj, nFlag))
		return FALSE;
	if (m_listMember.GetCount() != pObj->GetListMember().GetCount())
		return FALSE;
	CStringList& l1 = pObj->GetListMember();

	POSITION pos = l1.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = l1.GetNext(pos);
		if (m_listMember.Find(strItem) == NULL)
			return FALSE;
	}
	pos = m_listMember.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = m_listMember.GetNext(pos);
		if (l1.Find(strItem) == NULL)
			return FALSE;
	}

	return TRUE;
}



void CaGroupDetail::Copy (const CaGroupDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	m_listMember.RemoveAll();
	CaGroup::Copy(c);
	CStringList& l1 = ((CaGroupDetail*)&c)->GetListMember();

	POSITION pos = l1.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = l1.GetNext(pos);
		m_listMember.AddTail(strItem);
	}
}
