/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlrule.cpp: implementation for the CaRule class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Rule object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlrule.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Rule 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaRule, CaDBObject, 1)
CaRule::CaRule(const CaRule& c)
{
	Copy (c);
}

CaRule CaRule::operator = (const CaRule& c)
{
	Copy (c);
	return *this;
}

void CaRule::Copy (const CaRule& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
}

void CaRule::Serialize (CArchive& ar)
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

BOOL CaRule::Matched (CaRule* pObj, MatchObjectFlag nFlag)
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
		if (!CaDBObject::Matched (pObj, nFlag))
			return FALSE;
		return TRUE;
	}
	return FALSE;
}


//
// Object: RULE Detail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaRuleDetail, CaRule, 1)
CaRuleDetail::CaRuleDetail (const CaRuleDetail& c)
{
	Copy (c);
}

CaRuleDetail CaRuleDetail::operator = (const CaRuleDetail& c)
{
	Copy (c);
	return *this;
}

	
void CaRuleDetail::Serialize (CArchive& ar)
{
	CaRule::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strBaseTable;
		ar << m_strBaseTableOwner;
		ar << m_strDetailText;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strBaseTable;
		ar >> m_strBaseTableOwner;
		ar >> m_strDetailText;
	}
}

BOOL CaRuleDetail::Matched (CaRuleDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME || nFlag == MATCHED_NAMExOWNER)
	{
		return CaRule::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (!CaRule::Matched (pObj, nFlag))
			return FALSE;
		if (pObj->GetDetailText().CompareNoCase(m_strDetailText) != 0)
			return FALSE;

		return TRUE;
	}
	return FALSE;
}


void CaRuleDetail::Copy (const CaRuleDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaRule::Copy(c);
	// 
	// Extra copy here:
	m_strDetailText = ((CaRuleDetail*)&c)->GetDetailText();
}

