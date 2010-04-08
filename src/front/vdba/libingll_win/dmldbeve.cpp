/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmldbeve.cpp: implementation for the CaDBEvent class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : DBEvent object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmldbeve.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: DBEvent
// ************************************************************************************************
IMPLEMENT_SERIAL (CaDBEvent, CaDBObject, 1)
CaDBEvent::CaDBEvent(const CaDBEvent& c)
{
	Copy (c);
}

CaDBEvent CaDBEvent::operator = (const CaDBEvent& c)
{
	Copy (c);
	return *this;
}

void CaDBEvent::Copy (const CaDBEvent& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBObject::Copy(c);
}


void CaDBEvent::Serialize (CArchive& ar)
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


BOOL CaDBEvent::Matched (CaDBEvent* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||  nFlag == MATCHED_NAMExOWNER)
	{
		return CaDBObject::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

//
// Object: CaDBEventDetail 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaDBEventDetail, CaDBEvent, 1)
CaDBEventDetail::CaDBEventDetail(const CaDBEventDetail& c)
{
	Copy(c);
}

CaDBEventDetail CaDBEventDetail::operator =(const CaDBEventDetail& c)
{
	Copy (c);
	return *this;
}

void CaDBEventDetail::Serialize (CArchive& ar)
{
	CaDBEvent::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strDetailText;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strDetailText;
	}
}

BOOL CaDBEventDetail::Matched (CaDBEventDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||  nFlag == MATCHED_NAMExOWNER)
	{
		return CaDBEvent::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (!CaDBEvent::Matched (pObj, nFlag))
			return FALSE;
		if (pObj->GetDetailText().CompareNoCase(m_strDetailText) != 0)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

void CaDBEventDetail::Copy (const CaDBEventDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBEvent::Copy(c);
	m_strDetailText = ((CaDBEventDetail*)&c)->GetDetailText();
}

