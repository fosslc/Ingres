/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlrole.cpp: Implementation for the CaRole class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Role object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlrole.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: CaRole
// ************************************************************************************************
IMPLEMENT_SERIAL (CaRole, CaDBObject, 1)
CaRole::CaRole(const CaRole& c)
{
	Copy (c);
}

CaRole CaRole::operator = (const CaRole& c)
{
	Copy (c);
	return *this;
}

void CaRole::Copy (const CaRole& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
}

void CaRole::Serialize (CArchive& ar)
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

BOOL CaRole::Matched (CaRole* pObj, MatchObjectFlag nFlag)
{
	if (pObj->GetItem().CompareNoCase(GetItem()) != 0)
		return FALSE;
	return TRUE;
}



//
// Object: CaRoleDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaRoleDetail, CaRole, 1)
CaRoleDetail::CaRoleDetail(const CaRoleDetail& c)
{
	Copy(c);
}

CaRoleDetail CaRoleDetail::operator = (const CaRoleDetail& c)
{
	Copy (c);
	return *this;
}

void CaRoleDetail::Serialize (CArchive& ar)
{
	CaRole::Serialize (ar);
	CaURPPrivileges::Serialize(ar);

	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_bExternalPassword;
/*
		ar << m_nPriveFlag;
		ar << m_nDefaultPriveFlag;
		ar << m_bAllEvent;
		ar << m_bQueryText;
		ar << m_strLimitSecurityLabel;
*/
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_bExternalPassword;
/*
		ar >> m_nPriveFlag;
		ar >> m_nDefaultPriveFlag;
		ar >> m_bAllEvent;
		ar >> m_bQueryText;
		ar >> m_strLimitSecurityLabel;
*/
	}
}

BOOL CaRoleDetail::Matched (CaRoleDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaRole::Matched (pObj, nFlag))
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



void CaRoleDetail::Copy (const CaRoleDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaRole::Copy(c);

	m_nPriveFlag = c.m_nPriveFlag;
	m_nDefaultPriveFlag = c.m_nDefaultPriveFlag;
	m_bAllEvent = c.m_bAllEvent;
	m_bQueryText = c.m_bQueryText;
	m_strLimitSecurityLabel = c.m_strLimitSecurityLabel;
}

