/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmllocat.h: interface for the CaLocation class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Location object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmllocat.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: CaLocation
// ******************
IMPLEMENT_SERIAL (CaLocation, CaDBObject, 1)
CaLocation::CaLocation(const CaLocation& c)
{
	Copy (c);
}

CaLocation CaLocation::operator = (const CaLocation& c)
{
	Copy (c);
	return *this;
}

void CaLocation::Copy (const CaLocation& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
}

void CaLocation::Serialize (CArchive& ar)
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

BOOL CaLocation::Matched (CaLocation* pObj, MatchObjectFlag nFlag)
{
	if (pObj->GetName().CompareNoCase (m_strItem) != 0)
		return FALSE;
	return TRUE;
}


//
// Object: CaLocationDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaLocationDetail, CaLocation, 1)
CaLocationDetail::CaLocationDetail(const CaLocationDetail& c)
{
	Copy(c);
}

CaLocationDetail CaLocationDetail::operator = (const CaLocationDetail& c)
{
	Copy (c);
	return *this;
}

void CaLocationDetail::Init()
{
	m_tchDataUsage = _T('\0');
	m_tchJournalUsage = _T('\0');
	m_tchCheckPointUsage = _T('\0');
	m_tchWorkUsage = _T('\0');
	m_tchDumpUsage = _T('\0');
	m_strLocationArea = _T("");
}

void CaLocationDetail::Serialize (CArchive& ar)
{
	CaLocation::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_tchDataUsage;
		ar << m_tchJournalUsage;
		ar << m_tchCheckPointUsage;
		ar << m_tchWorkUsage;
		ar << m_tchDumpUsage;
		ar << m_strLocationArea;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_tchDataUsage;
		ar >> m_tchJournalUsage;
		ar >> m_tchCheckPointUsage;
		ar >> m_tchWorkUsage;
		ar >> m_tchDumpUsage;
		ar >> m_strLocationArea;
	}
}

BOOL CaLocationDetail::Matched (CaLocationDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaLocation::Matched (pObj, nFlag))
		return FALSE;
	if (m_tchDataUsage != pObj->GetDataUsage())
		return FALSE;
	if (m_tchJournalUsage != pObj->GetJournalUsage())
		return FALSE;
	if (m_tchCheckPointUsage != pObj->GetCheckPointUsage())
		return FALSE;
	if (m_tchWorkUsage != pObj->GetWorkUsage())
		return FALSE;
	if (m_tchDumpUsage != pObj->GetDumpUsage())
		return FALSE;
	if (m_strLocationArea.CompareNoCase(pObj->GetLocationArea()) != 0)
		return FALSE;

	return TRUE;
}



void CaLocationDetail::Copy (const CaLocationDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaLocation::Copy(c);

	m_tchDataUsage = c.m_tchDataUsage;
	m_tchJournalUsage = c.m_tchJournalUsage;
	m_tchCheckPointUsage = c.m_tchCheckPointUsage;
	m_tchWorkUsage = c.m_tchWorkUsage;
	m_tchDumpUsage = c.m_tchDumpUsage;
	m_strLocationArea = c.m_strLocationArea;
}

