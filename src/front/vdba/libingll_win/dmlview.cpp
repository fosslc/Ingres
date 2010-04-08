/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlview.cpp: implementation of the CaView class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : View object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 17-Mar-2003 (schph01)
**    SIR #109220 Add data member m_nFlag to manage procedure in Star Database
**/

#include "stdafx.h"
#include "dmlview.h"
#include "constdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: View
// ************************************************************************************************
IMPLEMENT_SERIAL (CaView, CaDBObject, 1)
CaView::CaView(const CaView& c)
{
	Copy (c);
}

CaView CaView::operator = (const CaView& c)
{
	Copy (c);
	return *this;
}

void CaView::Copy (const CaView& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	Initialize();
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_nFlag = c.m_nFlag;
}

void CaView::Serialize (CArchive& ar)
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

void CaView::Initialize()
{
	m_nObjectType = OBT_VIEW;
	m_nFlag = OBJTYPE_NOTSTAR;
}

BOOL CaView::Matched (CaView* pObj, MatchObjectFlag nFlag)
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
		if (pObj->GetFlag() != m_nFlag)
			return FALSE;
		return TRUE;
	}
	return FALSE;
}


//
// Object: CaViewDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaViewDetail, CaView, 1)
CaViewDetail::CaViewDetail (const CaViewDetail& c)
{
	Copy(c);
}

CaViewDetail CaViewDetail::operator = (const CaViewDetail& c)
{
	Copy (c);
	return *this;
}


BOOL CaViewDetail::Matched (CaViewDetail* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME || nFlag == MATCHED_NAMExOWNER)
	{
		return CaDBObject::Matched (pObj, nFlag);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (!CaDBObject::Matched (pObj, nFlag))
			return FALSE;
		if (m_strDetailText.CompareNoCase (pObj->GetDetailText()) != 0)
			return FALSE;
		ASSERT(FALSE); // compare list columns
		return TRUE;
	}
	return FALSE;
}


void CaViewDetail::Serialize (CArchive& ar)
{
	CaView::Serialize (ar);
	m_listColumn.Serialize(ar);
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

void CaViewDetail::Copy (const CaViewDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaView::Copy(c);
	m_strDetailText = c.GetDetailText();

	CTypedPtrList < CObList, CaColumn* >& lc = ((CaViewDetail*)&c)->GetListColumns();
	POSITION pos = lc.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pObj = lc.GetNext(pos);
		CaColumn* pNew = new CaColumn(*pObj);
		m_listColumn.AddTail(pNew);
	}
}

