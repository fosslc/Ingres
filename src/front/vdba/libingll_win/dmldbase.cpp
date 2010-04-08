/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmldbase.cpp: implementation for the CaDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object
**
** History:
**
** 05-Feb-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmldbase.h"
#include "constdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Database 
// ****************
IMPLEMENT_SERIAL (CaDatabase, CaDBObject, 1)
CaDatabase::CaDatabase(const CaDatabase& c):CaDBObject()
{
	Copy(c);
}

void CaDatabase::Copy(const CaDatabase& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBObject::Copy(c);
	m_nFlags           = c.m_nFlags;
	m_nService         = c.m_nService;
}

CaDatabase CaDatabase::operator = (const CaDatabase& c)
{
	Copy (c);
	return *this;
}


CaDatabase::~CaDatabase()
{
}

void CaDatabase::Initialize()
{
	SetObjectID(OBT_DATABASE);
	m_nFlags = 0;
	m_nService = 0;
}

void CaDatabase::SetReadOnly(BOOL bSet)
{
	if (bSet)
		m_nFlags |= DBFLAG_READONLY;
	else
		m_nFlags &=~DBFLAG_READONLY;
}

void CaDatabase::SetPrivate (BOOL bSet)
{
	if (bSet)
		m_nFlags &=~DBFLAG_GLOBAL;
	else
		m_nFlags |= DBFLAG_GLOBAL;
}

void CaDatabase::SetStar (int  nStar)
{
	switch (nStar)
	{
	case OBJTYPE_NOTSTAR:
		m_nService &=~DBFLAG_STARLINK;
		m_nService &=~DBFLAG_STARNATIVE;
		break;
	case OBJTYPE_STARNATIVE:
		m_nService &=~DBFLAG_STARLINK;
		m_nService |=DBFLAG_STARNATIVE;
		break;
	case OBJTYPE_STARLINK:
		m_nService &=~DBFLAG_STARNATIVE;
		m_nService |=DBFLAG_STARLINK;
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}


BOOL CaDatabase::IsReadOnly(){return (m_nFlags & DBFLAG_READONLY) != 0;}
BOOL CaDatabase::IsPrivate (){return (m_nFlags & DBFLAG_GLOBAL) == 0;}
int CaDatabase::GetStar()
{
	if ((m_nService & DBFLAG_STARLINK) != 0)
		return OBJTYPE_STARLINK;
	if ((m_nService & DBFLAG_STARNATIVE) != 0)
		return OBJTYPE_STARNATIVE;
	return OBJTYPE_NOTSTAR;
}


void CaDatabase::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nFlags;
		ar << m_nService;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nFlags;
		ar >> m_nService;
	}
}

BOOL CaDatabase::Matched (CaDatabase* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetName().CompareNoCase (m_strItem) == 0);
	}
	else
	if (nFlag == MATCHED_NAME ||  nFlag == MATCHED_NAMExOWNER)
	{
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;
		if (pObj->GetOwner().CompareNoCase (m_strItemOwner) != 0)
			return FALSE;
		return TRUE;
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;
		if (pObj->GetOwner().CompareNoCase (m_strItemOwner) != 0)
			return FALSE;
		if (pObj->GetOwner().CompareNoCase (m_strItemOwner) != 0)
			return FALSE;
		if (pObj->GetFlag() != m_nFlags)
			return FALSE;
		if (pObj->GetService() != m_nService)
			return FALSE;
		return TRUE;
	}

	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}




//
// Object: DatabaseDetail 
// **********************
IMPLEMENT_SERIAL (CaDatabaseDetail, CaDatabase, 1)
CaDatabaseDetail::CaDatabaseDetail(const CaDatabaseDetail& c)
{
	Copy(c);
}

CaDatabaseDetail CaDatabaseDetail::operator = (const CaDatabaseDetail& c)
{
	Copy (c);
	return *this;
}

void CaDatabaseDetail::Copy(const CaDatabaseDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDatabase::Copy(c);
	m_strLocDatabase   = c.m_strLocDatabase;
	m_strLocJournal    = c.m_strLocJournal;
	m_strLocWork       = c.m_strLocWork;
	m_strLocCheckPoint = c.m_strLocCheckPoint;
	m_strLocDump       = c.m_strLocDump;
	m_strCompatLevel   = c.m_strCompatLevel;
	m_strSecurityLabel   = c.m_strSecurityLabel;
	m_bUnicodeEnable     = c.m_bUnicodeEnable;
}

void CaDatabaseDetail::Initialize()
{
	//
	// Primary locations:
	m_strLocDatabase = _T("");
	m_strLocJournal = _T("");
	m_strLocWork = _T("");
	m_strLocCheckPoint = _T("");
	m_strLocDump = _T("");
	m_strCompatLevel = _T("");
	m_strSecurityLabel = _T("");
	m_bUnicodeEnable = FALSE;
	//
	// Extended locations:
}

BOOL CaDatabaseDetail::Matched (CaDatabaseDetail* pObj, MatchObjectFlag nFlagE)
{
	if (!CaDatabase::Matched(pObj, nFlagE))
		return FALSE;

	if (pObj->m_strLocDatabase.CompareNoCase(m_strLocDatabase) != 0)
		return FALSE;
	if (pObj->m_strLocJournal.CompareNoCase(m_strLocDatabase) != 0)
		return FALSE;
	if (pObj->m_strLocWork.CompareNoCase(m_strLocWork) != 0)
		return FALSE;
	if (pObj->m_strLocCheckPoint.CompareNoCase(m_strLocCheckPoint) != 0)
		return FALSE;
	if (pObj->m_strLocDump.CompareNoCase(m_strLocDump) != 0)
		return FALSE;
	if (pObj->m_strCompatLevel.CompareNoCase(m_strCompatLevel) != 0)
		return FALSE;
	if (pObj->m_strSecurityLabel.CompareNoCase(m_strSecurityLabel) != 0)
		return FALSE;
	return FALSE;
}


void CaDatabaseDetail::Serialize (CArchive& ar)
{
	CaDatabase::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		//
		// Primary locations:
		ar << m_strLocDatabase;
		ar << m_strLocJournal;
		ar << m_strLocWork;
		ar << m_strLocCheckPoint;
		ar << m_strLocDump;
		ar << m_strCompatLevel;
		ar << m_strSecurityLabel;
		ar << m_bUnicodeEnable;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		//
		// Primary locations:
		ar >> m_strLocDatabase;
		ar >> m_strLocJournal;
		ar >> m_strLocWork;
		ar >> m_strLocCheckPoint;
		ar >> m_strLocDump;
		ar >> m_strCompatLevel;
		ar >> m_strSecurityLabel;
		ar >> m_bUnicodeEnable;
	}
}
