/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlprofi.h: interface for the CaProfile class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Profile object 
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlprofi.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: CaProfile
// ************************************************************************************************
IMPLEMENT_SERIAL (CaProfile, CaDBObject, 1)
CaProfile::CaProfile(const CaProfile& c)
{
	Copy (c);
}

CaProfile CaProfile::operator = (const CaProfile& c)
{
	Copy (c);
	return *this;
}

void CaProfile::Copy (const CaProfile& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBObject::Copy(c);
}

void CaProfile::Serialize (CArchive& ar)
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


BOOL CaProfile::Matched (CaProfile* pObj, MatchObjectFlag nFlag)
{
	if (m_strItem.CompareNoCase(pObj->GetItem()) != 0)
			return FALSE;
	return TRUE;
}


//
// Object: CaProfileDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaProfileDetail, CaProfile, 1)
CaProfileDetail::CaProfileDetail(const CaProfileDetail& c)
{
	Copy(c);
}

CaProfileDetail CaProfileDetail::operator = (const CaProfileDetail& c)
{
	Copy (c);
	return *this;
}

void CaProfileDetail::Serialize (CArchive& ar)
{
	CaProfile::Serialize (ar);
	CaURPPrivileges::Serialize(ar);
	//
	// Serialize the extra data manage by user:
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();
/*
		ar << m_nPriveFlag;
		ar << m_nDefaultPriveFlag;
		ar << m_bAllEvent;
		ar << m_bQueryText;
		ar << m_strLimitSecurityLabel;
*/
		ar << m_strDefaultGroup;
		ar << m_strExpireDate;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;
/*
		ar >> m_nPriveFlag;
		ar >> m_nDefaultPriveFlag;
		ar >> m_bAllEvent;
		ar >> m_bQueryText;
		ar >> m_strLimitSecurityLabel;
*/
		ar >> m_strDefaultGroup;
		ar >> m_strExpireDate;
	}
}

BOOL CaProfileDetail::Matched (CaProfileDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaProfile::Matched (pObj, nFlag))
		return FALSE;
	if (pObj->GetDefaultGroup().CompareNoCase(m_strDefaultGroup) != 0)
		return FALSE;
	if (pObj->GetExpireDate().CompareNoCase(m_strExpireDate) != 0)
		return FALSE;
	if (pObj->GetLimitSecurityLabel().CompareNoCase(GetLimitSecurityLabel()) != 0)
		return FALSE;
	if (pObj->GetPrivelegeFlag() != GetPrivelegeFlag())
		return FALSE;
	if (pObj->GetDefaultPrivelegeFlag() != GetDefaultPrivelegeFlag())
		return FALSE;
	if (pObj->IsSecurityAuditAllEvent() != IsSecurityAuditAllEvent())
		return FALSE;
	if (pObj->IsSecurityAuditQueryText() != IsSecurityAuditQueryText())
		return FALSE;

	return TRUE;
}



void CaProfileDetail::Copy (const CaProfileDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaProfile::Copy(c);

	m_nPriveFlag = c.m_nPriveFlag;
	m_nDefaultPriveFlag= c.m_nDefaultPriveFlag;
	m_bAllEvent= c.m_bAllEvent;
	m_bQueryText= c.m_bQueryText;
	m_strLimitSecurityLabel= c.m_strLimitSecurityLabel;
	m_strDefaultGroup= c.m_strDefaultGroup;
	m_strExpireDate= c.m_strExpireDate;
}
