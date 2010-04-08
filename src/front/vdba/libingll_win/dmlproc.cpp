/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlproc.h: interface for the CaProcedure class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Procedure object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 17-Mar-2003 (schph01)
**    SIR #109220 Add data member m_nFlag to manage procedure in Star Database
** 30-Oct-2003 (noifr01)
**    fixed propagation error upon massive ingres30->main propagation
**/


#include "stdafx.h"
#include "dmlproc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Object: Procedure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaProcedure, CaDBObject, 1)
CaProcedure::CaProcedure(const CaProcedure& c)
{
	Copy (c);
}

CaProcedure CaProcedure::operator = (const CaProcedure& c)
{
	Copy (c);
	return *this;
}

void CaProcedure::Copy (const CaProcedure& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	CaDBObject::Copy(c);
	m_nFlag = c.m_nFlag;

}

void CaProcedure::Serialize (CArchive& ar)
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


BOOL CaProcedure::Matched (CaProcedure* pObj, MatchObjectFlag nFlag)
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
// Object: Procedure Detail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaProcedureDetail, CaProcedure, 1)
CaProcedureDetail::CaProcedureDetail (const CaProcedureDetail& c)
{
	Copy (c);
}


CaProcedureDetail CaProcedureDetail::operator = (const CaProcedureDetail& c)
{
	Copy (c);
	return *this;
}

void CaProcedureDetail::Serialize (CArchive& ar)
{
	CaProcedure::Serialize (ar);
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

BOOL CaProcedureDetail::Matched (CaProcedureDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||nFlag == MATCHED_NAMExOWNER)
	{
		return CaProcedure::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		if (!CaProcedure::Matched (pObj, nFlag))
			return FALSE;
		if (pObj->GetDetailText().CompareNoCase(m_strDetailText) != 0)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}


void CaProcedureDetail::Copy (const CaProcedureDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaProcedure::Copy(c);
	m_strDetailText = ((CaProcedureDetail*)&c)->GetDetailText();
}
