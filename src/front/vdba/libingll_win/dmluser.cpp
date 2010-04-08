/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmluser.cpp: implementation for the CaUser class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : User object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmluser.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: User
// ************************************************************************************************
IMPLEMENT_SERIAL (CaUser, CaDBObject, 1)

CaUser::CaUser(const CaUser& c)
{
	Copy (c);
}

CaUser CaUser::operator = (const CaUser& c)
{
	Copy (c);
	return *this;
}

void CaUser::Copy (const CaUser& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
}

void CaUser::Init()
{
	SetObjectID(OBT_USER);;
}

void CaUser::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Serialize the properties of user:
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


BOOL CaUser::Matched (CaUser* pObj, MatchObjectFlag nFlag)
{
	return CaDBObject::Matched (pObj, nFlag);
}

//
// Object: UserDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaUserDetail, CaUser, 1)
CaUserDetail::CaUserDetail(const CaUserDetail& c)
{
	Copy(c);
}

CaUserDetail CaUserDetail::operator = (const CaUserDetail& c)
{
	Copy (c);
	return *this;
}

void CaUserDetail::Serialize (CArchive& ar)
{
	CaUser::Serialize (ar);
	CaURPPrivileges::Serialize(ar);
	//
	// Serialize the extra data manage by user:
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strDefaultGroup;
		ar << m_strDefaultProfile;
		ar << m_strExpireDate;
		ar << m_bExternalPassword;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strDefaultGroup;
		ar >> m_strDefaultProfile;
		ar >> m_strExpireDate;
		ar >> m_bExternalPassword;
	}
}

BOOL CaUserDetail::Matched (CaUserDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME || nFlag == MATCHED_NAMExOWNER)
	{
		return CaUser::Matched (pObj, nFlag);
	}
	else
	{
		if (!CaUser::Matched (pObj, nFlag))
			return FALSE;
		if (pObj->GetDefaultGroup().CompareNoCase(m_strDefaultGroup) != 0)
			return FALSE;
		if (pObj->GetDefaultProfile().CompareNoCase(m_strDefaultProfile) != 0)
			return FALSE;
		if (pObj->GetExpireDate().CompareNoCase(m_strExpireDate) != 0)
			return FALSE;
		if (pObj->GetLimitSecurityLabel().CompareNoCase(GetLimitSecurityLabel()) != 0)
			return FALSE;
		if (pObj->IsExternalPassword() != IsExternalPassword())
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
}



void CaUserDetail::Copy (const CaUserDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaUser::Copy(c);

	m_nPriveFlag = c.m_nPriveFlag;
	m_nDefaultPriveFlag = c.m_nDefaultPriveFlag;
	m_bAllEvent = c.m_bAllEvent;
	m_bQueryText = c.m_bQueryText;
	m_strLimitSecurityLabel = c.m_strLimitSecurityLabel;

	m_strDefaultGroup = c.m_strDefaultGroup;
	m_strDefaultProfile = c.m_strDefaultProfile;
	m_strExpireDate = c.m_strExpireDate;
	m_bExternalPassword = c.m_bExternalPassword;
}
