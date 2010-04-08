/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlgrant.cpp: Implementation for the CaGrant class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : CaGrantee object
**
** History:
**
** 24-Oct-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    Created
**/

#include "stdafx.h"
#include "dmlgrant.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: CaPrivilegeItem 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaPrivilegeItem, CaDBObject, 1)
CaPrivilegeItem::CaPrivilegeItem():CaDBObject(_T(""), _T(""))
{
	m_bPrivilege = FALSE;
	m_nLimitValue = -1;
}

CaPrivilegeItem::CaPrivilegeItem(LPCTSTR lpszName, BOOL bPrivilege):CaDBObject(lpszName, _T(""))
{
	m_strItem.MakeLower();
	m_bPrivilege = bPrivilege;
	m_nLimitValue = -1;
}

CaPrivilegeItem::CaPrivilegeItem(LPCTSTR lpszName, BOOL bPrivilege, int nVal):CaDBObject(lpszName, _T(""))
{
	m_strItem.MakeLower();
	m_bPrivilege = bPrivilege;
	m_nLimitValue = nVal;
}

CaPrivilegeItem::CaPrivilegeItem(const CaPrivilegeItem& c)
{
	Copy(c);
}

CaPrivilegeItem CaPrivilegeItem::operator =(const CaPrivilegeItem& c)
{
	Copy (c);
	return *this;
}

BOOL CaPrivilegeItem::Matched (CaPrivilegeItem* pObj)
{
	if (pObj->GetPrivilege().CompareNoCase(m_strItem) != 0)
		return FALSE;
	if (pObj->IsPrivilege() != m_bPrivilege)
		return FALSE;
	return TRUE;
}

void CaPrivilegeItem::Serialize (CArchive& ar)
{
	CaDBObject::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();
	
		ar << m_bPrivilege;
		ar << m_nLimitValue;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_bPrivilege;
		ar >> m_nLimitValue;
	}
}

void CaPrivilegeItem::Copy(const CaPrivilegeItem& c)
{
	CaDBObject::Copy(c);
	m_bPrivilege = c.m_bPrivilege;
	m_nLimitValue = c.m_nLimitValue;
}

CaPrivilegeItem* CaPrivilegeItem::FindPrivilege(LPCTSTR lpszName, BOOL bPrivilege, CTypedPtrList< CObList, CaPrivilegeItem* >& l)
{
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pObj = l.GetNext(pos);
		if (pObj->GetPrivilege().CompareNoCase(lpszName) == 0 && pObj->IsPrivilege() == bPrivilege)
			return pObj;
	}
	return NULL;
}

CaPrivilegeItem* CaPrivilegeItem::FindPrivilege(CaPrivilegeItem* pRivilege, CTypedPtrList< CObList, CaPrivilegeItem* >& l)
{
	return CaPrivilegeItem::FindPrivilege(pRivilege->GetPrivilege(), pRivilege->IsPrivilege(), l);
}

//
// Object: CaGrantee
// ************************************************************************************************
IMPLEMENT_SERIAL (CaGrantee, CaUser, 1)
CaGrantee::CaGrantee(const CaGrantee& c)
{
	Copy (c);
}

CaGrantee CaGrantee::operator = (const CaGrantee& c)
{
	Copy (c);
	return *this;
}

CaGrantee::~CaGrantee()
{
	while (!m_listPrivilege.IsEmpty())
		delete m_listPrivilege.RemoveHead();
}

void CaGrantee::Copy (const CaGrantee& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaUser::Copy(c);
	m_nRealIdentity = c.m_nRealIdentity;
	while (!m_listPrivilege.IsEmpty())
		delete m_listPrivilege.RemoveHead();

	CTypedPtrList< CObList, CaPrivilegeItem* >& l1 = ((CaGrantee*)&c)->GetListPrivilege();
	POSITION pos = l1.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pItem = l1.GetNext(pos);
		m_listPrivilege.AddTail(new CaPrivilegeItem(*pItem));
	}
}

void CaGrantee::Serialize (CArchive& ar)
{
	CaUser::Serialize (ar);
	m_listPrivilege.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nRealIdentity;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nRealIdentity;
	}
}

void CaGrantee::SetPrivilege(LPCTSTR lpszPriv, BOOL bPrivilege)
{
	CaPrivilegeItem* pPriv = new CaPrivilegeItem(lpszPriv, bPrivilege);
	m_listPrivilege.AddTail(pPriv);
}

void CaGrantee::SetPrivilege2(LPCTSTR lpszPriv, int nVal)
{
	CaPrivilegeItem* pPriv = new CaPrivilegeItem(lpszPriv, TRUE, nVal);
	m_listPrivilege.AddTail(pPriv);
}


BOOL CaGrantee::Matched (CaGrantee* pObj)
{
	if (pObj->GetName().CompareNoCase (m_strItem) != 0)
		return FALSE;
	if (m_listPrivilege.GetCount() != pObj->GetListPrivilege().GetCount())
		return FALSE;
	CTypedPtrList< CObList, CaPrivilegeItem* >& l1 = pObj->GetListPrivilege();

	POSITION pos = l1.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pItem = l1.GetNext(pos);
		if (!CaPrivilegeItem::FindPrivilege(pItem->GetPrivilege(), pItem->IsPrivilege(), m_listPrivilege))
			return FALSE;
	}

	pos = m_listPrivilege.GetHeadPosition();
	while (pos != NULL)
	{
		CaPrivilegeItem* pItem = m_listPrivilege.GetNext(pos);
		if (!CaPrivilegeItem::FindPrivilege(pItem->GetPrivilege(), pItem->IsPrivilege(), l1))
			return FALSE;
	}
	return TRUE;
}

