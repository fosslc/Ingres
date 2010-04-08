/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlcolum.cpp: implementation for the CaColumn class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Column object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 28-Feb-2003 (schph01)
**    SIR #109220, Manage compare "User-Defined Data Types" in VDDA
** 30-Oct-2003 (noifr01)
**    fixed propagation error upon massive ingres30->main propagation
** 17-May-2004 (uk$so01)
**    SIR #109220, Fix some bugs introduced by change #462417
**    (Manage compare "User-Defined Data Types" in VDDA)
** 18-May-2004 (schph01)
**    SIR 111507 Add management for new column type bigint(int8)
** 04-Feb-2008 (drivi01)
**    In efforts to port to Visual Studio 2008 compiler, update return
**    type for function CaColumn::IsUserDefinedDataTypes.
**/

#include "stdafx.h"
#include "dmlcolum.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Column 
// ***************
IMPLEMENT_SERIAL (CaColumn, CaDBObject, 1)
CaColumn::CaColumn(const CaColumn& c)
{
	Copy (c);
}

CaColumn CaColumn::operator = (const CaColumn& c)
{
	Copy (c);
	return *this;
}

void CaColumn::Copy (const CaColumn& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	m_iDataType  = c.m_iDataType;
	m_strDataType= c.m_strDataType;
	m_strLength  = c.m_strLength;
	m_bNullable  = c.m_bNullable;
	m_bDefault   = c.m_bDefault;
	m_bColumnText= c.m_bColumnText;
	m_strDefault = c.m_strDefault;
	m_nLength    = c.m_nLength;
	m_nScale     = c.m_nScale;
	m_nHasDefault= c.m_nHasDefault;
	m_nKeySequence        = c.m_nKeySequence;
	m_strInternalDataType = c.m_strInternalDataType;
	m_bSystemMaintained   = c.m_bSystemMaintained;
}

void CaColumn::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
	if (ar.IsStoring())
	{
		ar << m_iDataType;
		ar << m_strDataType;
		ar << m_strLength;
		ar << m_bNullable;
		ar << m_bDefault;
		ar << m_bColumnText;
		ar << m_strDefault;
		ar << m_nLength;
		ar << m_nScale;
		ar << m_nHasDefault;
		ar << m_nKeySequence;
		ar << m_strInternalDataType;
		ar << m_bSystemMaintained;
	}
	else
	{
		ar >> m_iDataType;
		ar >> m_strDataType;
		ar >> m_strLength;
		ar >> m_bNullable;
		ar >> m_bDefault;
		ar >> m_bColumnText;
		ar >> m_strDefault;
		ar >> m_nLength;
		ar >> m_nScale;
		ar >> m_nHasDefault;
		ar >> m_nKeySequence;
		ar >> m_strInternalDataType;
		ar >> m_bSystemMaintained;
	}
	
	if (!IstSerializeAll())
		return;
	//
	// Serialize the extra data manage by this object:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

BOOL CaColumn::Matched (CaColumn* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME ||
	    nFlag == MATCHED_NAMExOWNER)
	{
		return (pObj->GetName().CompareNoCase (m_strItem) == 0);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
		BOOL bOk = FALSE;
		if (pObj->GetName().CompareNoCase (m_strItem) != 0)
			return FALSE;

		//
		// TODO:
		ASSERT(FALSE);
		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}

int CaColumn::NeedLength()
{
	if (m_iDataType == -1)
	{
		CString strTemp = m_strDataType;
		strTemp.MakeLower();
		if (strTemp == _T("decimal"))
			return COLUMN_PRECISIONxSCALE;
		if (strTemp == _T("date")    || 
		    strTemp == _T("float4")  || 
		    strTemp == _T("float8")  || 
		    strTemp == _T("int1")    || 
		    strTemp == _T("int2")    || 
		    strTemp == _T("int4")    || 
		    strTemp == _T("int8")    || 
		    strTemp == _T("bigint")  || 
		    strTemp == _T("integer") || 
		    strTemp == _T("money"))
		{
			return COLUMN_NOLENGTH;
		}
		else
		if (strTemp == _T("float"))
			return COLUMN_PRECISION;
		else
			return COLUMN_LENGTH;
	}
	else
	{
		if (IsUserDefinedDataTypes()) // User-Defined Data Type or OBJECT_KEY or TABLE_KEY
			return COLUMN_NOLENGTH;
		int nNeed = INGRESII_llNeedLength(m_iDataType);
		switch (nNeed)
		{
		case 1:
			return COLUMN_LENGTH;
		case 2:
			return COLUMN_PRECISION;
		case 3:
			return COLUMN_PRECISIONxSCALE;
		default:
			return COLUMN_NOLENGTH;
		}
	}
}



void CaColumn::GenerateStatement (CString& strColumText)
{
//	if (m_bShortDisplay)
//	{
//		strColumText.Format(_T("%s %s"), (LPCTSTR)m_strItem, (LPCTSTR)m_strDataType);
//	}
//	else
	{
		int nNeedLength = NeedLength();
		switch (nNeedLength)
		{
		case CaColumn::COLUMN_LENGTH:
		case CaColumn::COLUMN_PRECISION:
			ASSERT (m_nLength > 0);
			if (m_nLength > 0)
				m_strLength.Format (_T("%d"), m_nLength);
			break;
		case CaColumn::COLUMN_PRECISIONxSCALE:
			ASSERT (m_nLength > 0);
			if (m_nLength > 0)
				m_strLength.Format (_T("%d, %d"), m_nLength, m_nScale);
			break;
		default:
			break;
		}
		if (IsUserDefinedDataTypes())
		{
			strColumText.Format (
				_T("%s %s"), 
				(LPCTSTR)INGRESII_llQuoteIfNeeded(m_strItem), 
				(LPCTSTR)m_strInternalDataType);
			if (m_strInternalDataType.CompareNoCase(_T("object_key")) == 0 ||
				m_strInternalDataType.CompareNoCase(_T("table_key")) == 0 )
			{
				if ( IsSystemMaintained() )
					strColumText += _T(" (system_maintained)");
				else
					strColumText += _T(" (not_system_maintained)");
			}
		}
		else
			strColumText.Format (
				_T("%s %s"), 
				(LPCTSTR)INGRESII_llQuoteIfNeeded(m_strItem), 
				(LPCTSTR)m_strDataType);

		if (!m_strLength.IsEmpty())
		{
			strColumText += _T("(");
			strColumText += m_strLength;
			strColumText += _T(") ");
		}
		else
			strColumText += _T(" ");

		if (m_bNullable)
			strColumText += _T("with null ");
		else
			strColumText += _T("not null ");

		if (m_bDefault)
		{
			if (!m_strDefault.IsEmpty())
			{
				strColumText += _T("default ");
				strColumText += m_strDefault;
				strColumText += _T(" ");
			}
			else
			{
				strColumText += _T("with default ");
			}
		}
		else
			strColumText += _T("not default ");
	}
	strColumText.MakeLower();
}


CaColumn* CaColumn::FindColumn(LPCTSTR lpszName, CTypedPtrList< CObList, CaColumn* >& listColumn)
{
	POSITION pos = listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = listColumn.GetNext(pos);
		if (pCol->GetName().CompareNoCase(lpszName) == 0)
			return pCol;
	}
	return NULL;
}

BOOL CaColumn::IsUserDefinedDataTypes ()
{
	// m_strInternalDataType always filled with the DBMS information when it is  queried from 
	// the low level (Get detail information)
	if (!m_strInternalDataType.IsEmpty() && m_strDataType.CompareNoCase(m_strInternalDataType) != 0)
		return TRUE;
	else
		return FALSE;
}

