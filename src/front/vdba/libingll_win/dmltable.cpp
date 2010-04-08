/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmltable.cpp: Implementation of the CaTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
*** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 27-Feb-2003 (schph01)
**    SIR #109220 - Manage compare locations in VDDA
                  - Add data member m_nFlag in CaTable to manage table in Star database.
                  - Add data members in CaTableDetail :
                        - m_bTablePhysInConsistent
                        - m_bTableLogInConsistent
                        - m_bTableRecoveryDisallowed
** 17-May-2004 (uk$so01)
**    SIR #109220, Fix some bugs introduced by change #462417
*     (Manage compare "User-Defined Data Types" in VDDA)
*/

#include "stdafx.h"
#include "dmltable.h"
#include "ingobdml.h"
#include "constdef.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Object: IndexOption object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaIndexOption, CObject, 1)
CaIndexOption::CaIndexOption()
{
	m_nFillFactor = 0; 
	m_nMinPage = 0;
	m_nMaxPage = 0;
	m_nLeafFill = 0;
	m_nNonLeafFill = 0;
	m_strStructure = _T("");
	m_nAllocation = 0;
	m_nExtend = 0;
}

CaIndexOption::CaIndexOption(const CaIndexOption& c)
{
	Copy (c);
}

CaIndexOption CaIndexOption::operator = (const CaIndexOption& c)
{
	Copy (c);
	return *this;
}

void CaIndexOption::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nFillFactor; 
		ar << m_nMinPage;
		ar << m_nMaxPage;
		ar << m_nLeafFill;
		ar << m_nNonLeafFill;
		ar << m_strStructure;
		ar << m_nAllocation;
		ar << m_nExtend;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nFillFactor; 
		ar >> m_nMinPage;
		ar >> m_nMaxPage;
		ar >> m_nLeafFill;
		ar >> m_nNonLeafFill;
		ar >> m_strStructure;
		ar >> m_nAllocation;
		ar >> m_nExtend;
	}
}

void CaIndexOption::Copy(const CaIndexOption& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	m_nFillFactor   = ((CaIndexOption*)&c)->GetFillFactor();
	m_nMinPage      = ((CaIndexOption*)&c)->GetMinPage();
	m_nMaxPage      = ((CaIndexOption*)&c)->GetMaxPage();
	m_nLeafFill     = ((CaIndexOption*)&c)->GetLeafFill();
	m_nNonLeafFill  = ((CaIndexOption*)&c)->GetNonLeafFill();
	m_strStructure  = ((CaIndexOption*)&c)->GetStructure();
	m_nAllocation   = ((CaIndexOption*)&c)->GetAllocation();
	m_nExtend       = ((CaIndexOption*)&c)->GetExtend();
}


//
// Object: key column object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaColumnKey, CObject, 1)
CaColumnKey::CaColumnKey(const CaColumnKey& c)
{
	Copy (c);
}

CaColumnKey CaColumnKey::operator = (const CaColumnKey& c)
{
	Copy (c);
	return *this;
}

void CaColumnKey::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_iDataType;
		ar << m_nLength;
		ar << m_nScale;
		ar << m_nOrder;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_iDataType;
		ar >> m_nLength;
		ar >> m_nScale;
		ar >> m_nOrder;
	}
}

void CaColumnKey::Copy(const CaColumnKey& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	m_iDataType = ((CaColumnKey*)&c)->GetType();
	m_nLength   = ((CaColumnKey*)&c)->GetLength();
	m_nScale    = ((CaColumnKey*)&c)->GetScale();
	m_nOrder    = ((CaColumnKey*)&c)->GetKeyOrder();
}

CaColumnKey* CaColumnKey::FindColumn(LPCTSTR lpszName, CTypedPtrList< CObList, CaColumnKey* >& listColumn)
{
	POSITION pos = listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnKey* pCol = listColumn.GetNext(pos);
		if (pCol->GetName().CompareNoCase(lpszName) == 0)
			return pCol;
	}
	return NULL;
}


//
// Object: Primary key object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaPrimaryKey, CaDBObject, 1)
CaPrimaryKey::CaPrimaryKey(const CaPrimaryKey& c)
{
	Copy(c);
}

CaPrimaryKey CaPrimaryKey::operator = (const CaPrimaryKey& c)
{
	Copy (c);
	return *this;
}

void CaPrimaryKey::Serialize (CArchive& ar)
{
	CaDBObject::Serialize(ar);
	m_constraintWithClause.Serialize(ar);
	m_listColumn.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strConstraintName;
		ar << m_strConstraintText;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strConstraintName;
		ar >> m_strConstraintText;
	}
}

void CaPrimaryKey::Copy(const CaPrimaryKey& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBObject::Copy(c);
	m_strConstraintName    = ((CaPrimaryKey*)&c)->GetConstraintName();
	m_strConstraintText    = ((CaPrimaryKey*)&c)->GetConstraintText();
	m_constraintWithClause = ((CaPrimaryKey*)&c)->GetWithClause();

	CTypedPtrList < CObList, CaColumnKey* >& lc = ((CaPrimaryKey*)&c)->GetListColumn();
	POSITION pos = lc.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnKey* pCol = lc.GetNext (pos);
		CaColumnKey* pNew = new CaColumnKey (*pCol);
		m_listColumn.AddTail(pNew);
	}
}

//
// Object: Unique key object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaUniqueKey, CaPrimaryKey, 1)
CaUniqueKey* CaUniqueKey::FindUniqueKey(CTypedPtrList < CObList, CaUniqueKey* >& l, CaUniqueKey* pKey)
{
	CString strKeyText = pKey->GetConstraintText();
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaUniqueKey* pK= l.GetNext(pos);
		if (strKeyText.CompareNoCase(pK->GetConstraintText()) == 0)
			return pK;
	}
	return NULL;
}

//
// Object: Foreign key object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaForeignKey, CaPrimaryKey, 1)
CaForeignKey::CaForeignKey(const CaForeignKey& c)
{
	Copy (c);
}

CaForeignKey CaForeignKey::operator = (const CaForeignKey& c)
{
	Copy (c);
	return *this;
}

void CaForeignKey::Serialize (CArchive& ar)
{
	CaPrimaryKey::Serialize(ar);
	m_listReferedColumn.Serialize(ar);
	m_constraintWithClause.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strReferedConstraintName;
		ar << m_strReferedTable;
		ar << m_strReferedTableOwner;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strReferedConstraintName;
		ar >> m_strReferedTable;
		ar >> m_strReferedTableOwner;
	}
}

void CaForeignKey::Copy(const CaForeignKey& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaPrimaryKey::Copy(c);
	m_strReferedConstraintName = ((CaForeignKey*)&c)->GetReferedConstraintName();
	m_strReferedTable          = ((CaForeignKey*)&c)->GetReferedTable();
	m_strReferedTableOwner     = ((CaForeignKey*)&c)->GetReferedTableOwner();
	m_constraintWithClause     = ((CaForeignKey*)&c)->GetWithClause();

	CTypedPtrList < CObList, CaColumnKey* >& lc = ((CaForeignKey*)&c)->GetReferedListColumn();
	POSITION pos = lc.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumnKey* pCol = lc.GetNext (pos);
		CaColumnKey* pNew = new CaColumnKey (*pCol);
		m_listReferedColumn.AddTail(pNew);
	}
}

CaForeignKey* CaForeignKey::FindForeignKey(CTypedPtrList < CObList, CaForeignKey* >& l, CaForeignKey* pKey)
{
	CString strKeyText = pKey->GetConstraintText();
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaForeignKey* pK= l.GetNext(pos);
		if (strKeyText.CompareNoCase(pK->GetConstraintText()) == 0)
			return pK;
	}
	return NULL;
}



//
// Object: Check key object 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaCheckKey, CaDBObject, 1)
CaCheckKey::CaCheckKey(const CaCheckKey& c)
{
	Copy(c);
}

CaCheckKey CaCheckKey::operator = (const CaCheckKey& c)
{
	Copy(c);
	return *this;
}

void CaCheckKey::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	m_constraintWithClause.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strConstraintName;
		ar << m_strStatement;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strConstraintName;
		ar >> m_strStatement;
	}
}

void CaCheckKey::Copy(const CaCheckKey& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaDBObject::Copy(c);
	m_strConstraintName   = ((CaCheckKey*)&c)->GetConstraintName();
	m_strStatement        = ((CaCheckKey*)&c)->GetConstraintText();
	m_constraintWithClause= ((CaCheckKey*)&c)->GetWithClause();
}

CaCheckKey* CaCheckKey::FindCheckKey(CTypedPtrList < CObList, CaCheckKey* >& l, CaCheckKey* pKey)
{
	CString strKeyText = pKey->GetConstraintText();
	POSITION pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaCheckKey* pK= l.GetNext(pos);
		if (strKeyText.CompareNoCase(pK->GetConstraintText()) == 0)
			return pK;
	}
	return NULL;
}



//
// Object: CaStorageStructure 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaStorageStructure, CaIndexOption, 1)
CaStorageStructure::CaStorageStructure(const CaStorageStructure& c)
{
	Copy(c);
}

CaStorageStructure CaStorageStructure::operator = (const CaStorageStructure& c)
{
	Copy(c);
	return *this;
}

void CaStorageStructure::Init()
{
	m_nPageSize = 0;
	m_cUniqueScope = _T(' ');
	m_cIsCompress = _T(' '); 
	m_cKeyCompress = _T(' '); 
	m_nAllocatedPage = 0;
	m_cUniqueRule = _T(' ');
	m_nCachePriority = 0;
}

void CaStorageStructure::Serialize (CArchive& ar)
{
	CaIndexOption::Serialize(ar);
	//
	// Serialize the extra data manage by this object:
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nPageSize;
		ar << m_cUniqueScope;
		ar << m_cIsCompress; 
		ar << m_cKeyCompress; 
		ar << m_nAllocatedPage;
		ar << m_cUniqueRule ;
		ar << m_nCachePriority;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nPageSize;
		ar >> m_cUniqueScope;
		ar >> m_cIsCompress; 
		ar >> m_cKeyCompress; 
		ar >> m_nAllocatedPage;
		ar >> m_cUniqueRule ;
		ar >> m_nCachePriority;
	}
}

void CaStorageStructure::Copy(const CaStorageStructure& c)
{
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaIndexOption::Copy (c);

	m_nPageSize      = ((CaStorageStructure*)&c)->GetPageSize();
	m_cUniqueScope   = ((CaStorageStructure*)&c)->GetUniqueScope();
	m_cIsCompress    = ((CaStorageStructure*)&c)->GetCompress(); 
	m_cKeyCompress   = ((CaStorageStructure*)&c)->GetKeyCompress(); 
	m_nAllocatedPage = ((CaStorageStructure*)&c)->GetAllocatedPage();
	m_cUniqueRule    = ((CaStorageStructure*)&c)->GetUniqueRule();
	m_nCachePriority = ((CaStorageStructure*)&c)->GetPriorityCache();
}

//
// Object: Table 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaTable, CaDBObject, 1)
CaTable::~CaTable()
{

}

CaTable::CaTable(const CaTable& c)
{
	Copy (c);
}

CaTable CaTable::operator = (const CaTable& c)
{
	Copy (c);
	return *this;
}

void CaTable::Copy (const CaTable& c)
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


void CaTable::Initialize()
{
	m_nObjectType = OBT_TABLE;
	m_nFlag = OBJTYPE_NOTSTAR;
}


void CaTable::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
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

BOOL CaTable::Matched (CaTable* pObj, MatchObjectFlag nFlag)
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
// Object: TableDetail
// ************************************************************************************************
IMPLEMENT_SERIAL (CaTableDetail, CaTable, 1)
CaTableDetail::~CaTableDetail()
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	while (!m_listUniqueKey.IsEmpty())
		delete m_listUniqueKey.RemoveHead();
	while (!m_listForeignKey.IsEmpty())
		delete m_listForeignKey.RemoveHead();
	while (!m_listCheckKey.IsEmpty())
		delete m_listCheckKey.RemoveHead();
}

CaTableDetail::CaTableDetail(const CaTableDetail& c)
{
	Copy (c);
}

CaTableDetail CaTableDetail::operator = (const CaTableDetail& c)
{
	Copy (c);
	return *this;
}

void CaTableDetail::Copy (const CaTableDetail& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaTable::Copy(c);

	m_strComment         = ((CaTableDetail*)&c)->GetComment();
	m_cJournaling        = ((CaTableDetail*)&c)->GetJournaling();
	m_cReadOnly          = ((CaTableDetail*)&c)->GetReadOnly();
	m_cDuplicatedRows    = ((CaTableDetail*)&c)->GetDuplicatedRows();
	m_nEstimatedRowCount = ((CaTableDetail*)&c)->GetEstimatedRowCount();
	m_strExpireDate      = ((CaTableDetail*)&c)->GetExpireDate();
	m_storageStructure   = ((CaTableDetail*)&c)->GetStorageStructure();
	m_primaryKey         = ((CaTableDetail*)&c)->GetPrimaryKey();

	CStringList& lev = ((CaTableDetail*)&c)->GetListLocation();
	POSITION pos = lev.GetHeadPosition();
	while (pos != NULL)
	{
		CString strEv = lev.GetNext(pos);
		m_listLocation.AddTail(strEv);
	}

	CTypedPtrList < CObList, CaUniqueKey* >& lu = ((CaTableDetail*)&c)->GetUniqueKeys();
	pos = lu.GetHeadPosition();
	while (pos != NULL)
	{
		CaUniqueKey* pObj = lu.GetNext(pos);
		CaUniqueKey* pNew = new CaUniqueKey(*pObj);
		m_listUniqueKey.AddTail(pNew);
	}

	CTypedPtrList < CObList, CaForeignKey* >& lf = ((CaTableDetail*)&c)->GetForeignKeys();
	pos = lf.GetHeadPosition();
	while (pos != NULL)
	{
		CaForeignKey* pObj = lf.GetNext(pos);
		CaForeignKey* pNew = new CaForeignKey(*pObj);
		m_listForeignKey.AddTail(pNew);
	}

	CTypedPtrList < CObList, CaCheckKey* >& lch = ((CaTableDetail*)&c)->GetCheckKeys();
	pos = lch.GetHeadPosition();
	while (pos != NULL)
	{
		CaCheckKey* pObj = lch.GetNext(pos);
		CaCheckKey* pNew = new CaCheckKey(*pObj);
		m_listCheckKey.AddTail(pNew);
	}

	CTypedPtrList < CObList, CaColumn* >& lc = ((CaTableDetail*)&c)->GetListColumns();
	pos = lc.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pObj = lc.GetNext(pos);
		CaColumn* pNew = new CaColumn(*pObj);
		m_listColumn.AddTail(pNew);
	}
}


void CaTableDetail::Initialize()
{
	m_nQueryFlag = DTQUERY_PROPERTY;

	m_strComment = _T("");
	m_cJournaling=_T(' ');
	m_cReadOnly = _T(' ');
	m_cDuplicatedRows = _T(' ');
	m_nEstimatedRowCount = -1;
	m_strExpireDate = _T("");
	if (!m_listLocation.IsEmpty())
		m_listLocation.RemoveAll();
	m_bTablePhysInConsistent   = FALSE;
	m_bTableLogInConsistent    = FALSE;
	m_bTableRecoveryDisallowed = FALSE;
}


void CaTableDetail::Serialize (CArchive& ar)
{
	CaTable::Serialize (ar);
	m_storageStructure.Serialize(ar);
	m_primaryKey.Serialize(ar);
	m_listUniqueKey.Serialize(ar);
	m_listForeignKey.Serialize(ar);
	m_listCheckKey.Serialize(ar);
	m_listColumn.Serialize(ar);
	m_listLocation.Serialize(ar);

	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strComment;
		ar << m_cJournaling;
		ar << m_cReadOnly;
		ar << m_cDuplicatedRows;
		ar << m_nEstimatedRowCount;
		ar << m_strExpireDate;

		ar << m_bTablePhysInConsistent;
		ar << m_bTableLogInConsistent;
		ar << m_bTableRecoveryDisallowed;

	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strComment;
		ar >> m_cJournaling;
		ar >> m_cReadOnly;
		ar >> m_cDuplicatedRows;
		ar >> m_nEstimatedRowCount;
		ar >> m_strExpireDate;
		ar >> m_bTablePhysInConsistent;
		ar >> m_bTableLogInConsistent;
		ar >> m_bTableRecoveryDisallowed;
	}
}

//
// TODO: complete in more detail ...
BOOL CaTableDetail::GenerateSyntaxCreate(CString& strSyntax)
{
	BOOL bOne = TRUE;
	CString strColumText;
	CString strColList = _T("");
	POSITION pos = m_listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = m_listColumn.GetNext (pos);
		pCol->GenerateStatement (strColumText);
		if (!bOne)
			strColList += _T(", ");

		strColList += strColumText;
		bOne = FALSE;
	}

	CString strSchema = _T("");
	if (!m_strItemOwner.IsEmpty())
		strSchema.Format (_T("%s."), (LPCTSTR)INGRESII_llQuoteIfNeeded(m_strItemOwner));
	strSyntax.Format (
		_T("create table %s%s (%s) "), 
		(LPCTSTR)strSchema,
		(LPCTSTR)INGRESII_llQuoteIfNeeded(m_strItem),
		(LPCTSTR)strColList);
	BOOL bWithClause = FALSE;
	CaStorageStructure& stg = GetStorageStructure();

	if (stg.GetPageSize() > 0)
		bWithClause = TRUE;

	if (bWithClause)
		strSyntax += _T("with ");

	if (stg.GetPageSize() > 0)
	{
		CString strPageSize;
		strPageSize.Format (_T("page_size = %d"), stg.GetPageSize());
		strSyntax += strPageSize;
	}
	return TRUE;
}

void  CaTableDetail::GenerateListLocation(CString& csListLoc)
{
	csListLoc.Empty();
	POSITION pos = m_listLocation.GetHeadPosition();
	while (pos != NULL)
	{
		csListLoc += m_listLocation.GetNext(pos);
		if (pos != NULL)
			csListLoc +=_T(", ");
	}
}
