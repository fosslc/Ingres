/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmloburp.cpp, Implementation File 
**    Project  : COM Server/Library
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data manipulation (User, Role, Profile)
**
** History:
**
** 26-Oct-1999 (uk$so01)
**    Created, 
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmloburp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaURPPrivileges::CaURPPrivileges()
{
	Init();
}

void CaURPPrivileges::Init()
{
	m_nPriveFlag = 0;
	m_nDefaultPriveFlag = 0;

	// Security Audit (all_event, default_event, query_text):
	BOOL m_bAllEvent = FALSE;
	BOOL m_bQueryText = FALSE;
	// Limit Security Label:
	m_strLimitSecurityLabel = _T("");
}

#define CaURPPrivileges_vers 1
void CaURPPrivileges::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << CaURPPrivileges_vers;

		ar << m_nPriveFlag;
		ar << m_nDefaultPriveFlag;
		ar << m_bAllEvent;
		ar << m_bQueryText;
		ar << m_strLimitSecurityLabel;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nPriveFlag;
		ar >> m_nDefaultPriveFlag;
		ar >> m_bAllEvent;
		ar >> m_bQueryText;
		ar >> m_strLimitSecurityLabel;
	}
}


BOOL CaURPPrivileges::IsPrivilege (UserRoleProfleFlags nFlag)
{
	BOOL bPriv = (m_nPriveFlag & nFlag)? TRUE: FALSE;
	return bPriv;
}

BOOL CaURPPrivileges::IsDefaultPrivilege(UserRoleProfleFlags nFlag)
{
	BOOL bPriv = (m_nDefaultPriveFlag & nFlag)? TRUE: FALSE;
	return bPriv;

}

void CaURPPrivileges::SetPrivilege(UserRoleProfleFlags nFlag)
{
	m_nPriveFlag |= nFlag;
}

void CaURPPrivileges::SetDefaultPrivilege(UserRoleProfleFlags nFlag)
{
	m_nDefaultPriveFlag |= nFlag;
}

void CaURPPrivileges::DelPrivilege(UserRoleProfleFlags nFlag)
{
	m_nPriveFlag &= ~nFlag;
}


void CaURPPrivileges::DelDefaultPrivilege(UserRoleProfleFlags nFlag)
{
	m_nDefaultPriveFlag &= ~nFlag;
}















