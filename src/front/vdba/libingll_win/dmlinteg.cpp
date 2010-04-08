/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlinteg.cpp: implementation for the CaIntegrity class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Integrity object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlinteg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Integrity
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIntegrity, CaDBObject, 1)
CaIntegrity::CaIntegrity(): CaDBObject()
{
	SetObjectID(OBT_INTEGRITY);
}

CaIntegrity::CaIntegrity(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem )
    : CaDBObject(lpszName, lpszOwner, bSystem)
{
	SetObjectID(OBT_INTEGRITY);
}

CaIntegrity::CaIntegrity(const CaIntegrity& c)
{
	Copy (c);
}

CaIntegrity CaIntegrity::operator = (const CaIntegrity& c)
{
	Copy (c);
	return *this;
}

void CaIntegrity::Copy (const CaIntegrity& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	m_nNumber = c.m_nNumber;
	m_strDetailText = c.m_strDetailText;
}

void CaIntegrity::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nNumber;
		ar << m_strDetailText;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nNumber;
		ar >> m_strDetailText;
	}
}


BOOL CaIntegrity::Matched (CaIntegrity* pObj, MatchObjectFlag nFlag)
{
	if (!CaDBObject::Matched (pObj, nFlag))
		return FALSE;
	if (pObj->GetDetailText().CompareNoCase (m_strDetailText) != 0)
		return FALSE;
	if (pObj->GetNumber() != m_nNumber)
		return FALSE;
	return TRUE;
}


//
// Object: IntegrityDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIntegrityDetail, CaIntegrity, 1)
CaIntegrityDetail::CaIntegrityDetail(const CaIntegrityDetail& c)
{
	Copy (c);
}

CaIntegrityDetail CaIntegrityDetail::operator = (const CaIntegrityDetail& c)
{
	Copy (c);
	return *this;
}

void CaIntegrityDetail::Copy (const CaIntegrityDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	CaIntegrity::Copy(c);
}

void CaIntegrityDetail::Serialize (CArchive& ar)
{
	CaIntegrity::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();
		ar << m_strBaseTable;
		ar << m_strBaseTableOwner;


	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strBaseTable;
		ar >> m_strBaseTableOwner;
	}
}


BOOL CaIntegrityDetail::Matched (CaIntegrityDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaIntegrity::Matched (pObj, nFlag))
		return FALSE;
	return TRUE;
}

