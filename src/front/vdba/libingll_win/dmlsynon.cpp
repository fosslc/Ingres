/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlsynon.cpp: implementation for the CaSynonym class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : CaSynonym object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlsynon.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: Synonym:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaSynonym, CaDBObject, 1)
CaSynonym::CaSynonym(const CaSynonym& c)
{
	Copy (c);
}

CaSynonym CaSynonym::operator = (const CaSynonym& c)
{
	Copy (c);
	return *this;
}

void CaSynonym::Copy (const CaSynonym& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
}

void CaSynonym::Serialize (CArchive& ar)
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

BOOL CaSynonym::Matched (CaSynonym* pObj, MatchObjectFlag nFlag)
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
// Object: CaSynonymDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaSynonymDetail, CaSynonym, 1)
CaSynonymDetail::CaSynonymDetail (const CaSynonymDetail& c)
{
	Copy (c);
}

CaSynonymDetail CaSynonymDetail::operator = (const CaSynonymDetail& c)
{
	Copy (c);
	return *this;
}


BOOL CaSynonymDetail::Matched (CaSynonymDetail* pObj, MatchObjectFlag nFlag)
{
	if (!CaSynonym::Matched(pObj, nFlag))
		return FALSE;

	if (nFlag == MATCHED_ALL)
	{
		if (pObj->GetOnType() != m_cOnType)
			return FALSE; 
		if (pObj->GetOnObject().CompareNoCase(m_strOnObject) != 0)
			return FALSE;
		if (pObj->GetOnObjectOwner().CompareNoCase(m_strOnObjectOwner) != 0)
			return FALSE;
		return TRUE;
	}
	return TRUE;
}

void CaSynonymDetail::Serialize (CArchive& ar)
{
	CaSynonym::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_cOnType; 
		ar << m_strOnObject;
		ar << m_strOnObjectOwner;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_cOnType; 
		ar >> m_strOnObject;
		ar >> m_strOnObjectOwner;
	}
}

void CaSynonymDetail::Copy (const CaSynonymDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaSynonym::Copy(c);

	m_cOnType          = ((CaSynonymDetail*)&c)->GetOnType();
	m_strOnObject      = ((CaSynonymDetail*)&c)->GetOnObject();
	m_strOnObjectOwner = ((CaSynonymDetail*)&c)->GetOnObjectOwner();
}

void CaSynonymDetail::Init()
{
	m_cOnType = _T('\0'); 
	m_strOnObject = _T("");
	m_strOnObjectOwner = _T("");
}
