/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlseq.h: interface for the CaSequence class
**    Project  : Meta data library 
**    Author   : Philippe Schalk (schph01)
**    Purpose  : Sequence object
**
** History:
**
** 22-Apr-2003 (schph01)
**    SIR #107523 Add Sequence Object
**/


#include "stdafx.h"
#include "dmlseq.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Object: Sequence 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaSequence, CaDBObject, 1)
CaSequence::CaSequence(const CaSequence& c)
{
	Copy (c);
}

CaSequence CaSequence::operator = (const CaSequence& c)
{
	Copy (c);
	return *this;
}

void CaSequence::Copy (const CaSequence& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	CaDBObject::Copy(c);
	m_nFlag = c.m_nFlag;

}

void CaSequence::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();
		ar << m_nFlag;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;
		ar >> m_nFlag;
	}
}


BOOL CaSequence::Matched (CaSequence* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||nFlag == MATCHED_NAMExOWNER)
	{
		return CaDBObject::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		if (!CaDBObject::Matched (pObj, nFlag))
			return FALSE;
		if (m_nFlag != pObj->GetFlag())
			return FALSE;
		return TRUE;
	}
	return FALSE;
}


//
// Object: Sequence Detail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaSequenceDetail, CaSequence, 1)
CaSequenceDetail::CaSequenceDetail (const CaSequenceDetail& c)
{
	Copy (c);
}


CaSequenceDetail CaSequenceDetail::operator = (const CaSequenceDetail& c)
{
	Copy (c);
	return *this;
}

void CaSequenceDetail::Serialize (CArchive& ar)
{
	CaSequence::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_bDecimalType;
		ar << m_bCycle;
		ar << m_bOrder;

		ar << m_csMaxValue;
		ar << m_csMinValue;
		ar << m_csStartWith;
		ar << m_csIncrementBy;
		ar << m_csNextValue;
		ar << m_csCacheSize;
		ar << m_csDecimalPrecision;

	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_bDecimalType;
		ar >> m_bCycle;
		ar >> m_bOrder;

		ar >> m_csMaxValue;
		ar >> m_csMinValue;
		ar >> m_csStartWith;
		ar >> m_csIncrementBy;
		ar >> m_csNextValue;
		ar >> m_csCacheSize;
		ar >> m_csDecimalPrecision;

	}
}

BOOL CaSequenceDetail::Matched (CaSequenceDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||nFlag == MATCHED_NAMExOWNER)
	{
		return CaSequence::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		if (!CaSequence::Matched (pObj, nFlag))
			return FALSE;

		if (pObj->GetDecimalType() != m_bDecimalType)
			return FALSE;

		if (pObj->GetDecimalType() && m_bDecimalType)
		{
			if (pObj->GetDecimalPrecision().CompareNoCase(m_csDecimalPrecision) != 0)
				return FALSE;
		}
		if (pObj->GetCycle() != m_bCycle)
			return FALSE;
		if (pObj->GetOrder() != m_bOrder)
			return FALSE;

		if (pObj->GetMaxValue().CompareNoCase(m_csMaxValue) != 0)
			return FALSE;

		if (pObj->GetMinValue().CompareNoCase(m_csMinValue) != 0)
			return FALSE;

		if (pObj->GetStartWith().CompareNoCase(m_csStartWith) != 0)
			return FALSE;

		if (pObj->GetIncrementBy().CompareNoCase(m_csIncrementBy) != 0)
			return FALSE;

		if (pObj->GetNextValue().CompareNoCase(m_csNextValue) != 0)
			return FALSE;

		if (pObj->GetCacheSize().CompareNoCase(m_csCacheSize) != 0)
			return FALSE;


		return TRUE;
	}
	return FALSE;
}


void CaSequenceDetail::Copy (const CaSequenceDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaSequence::Copy(c);

	m_bDecimalType  = ((CaSequenceDetail*)&c)->GetDecimalType();
	m_bCycle        = ((CaSequenceDetail*)&c)->GetCycle();
	m_bOrder        = ((CaSequenceDetail*)&c)->GetOrder();

	m_csMaxValue    = ((CaSequenceDetail*)&c)->GetMaxValue();
	m_csMinValue    = ((CaSequenceDetail*)&c)->GetMinValue();
	m_csStartWith   = ((CaSequenceDetail*)&c)->GetStartWith();
	m_csIncrementBy = ((CaSequenceDetail*)&c)->GetIncrementBy();
	m_csNextValue   = ((CaSequenceDetail*)&c)->GetNextValue();
	m_csCacheSize   = ((CaSequenceDetail*)&c)->GetCacheSize();
	m_csDecimalPrecision = ((CaSequenceDetail*)&c)->GetDecimalPrecision();
}
