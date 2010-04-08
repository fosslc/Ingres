/******************************************************************************
//                                                                           //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.         //
//                                                                           //
//    Source   : dmlindex.cpp Implementation file                            //
//                                                                           //
//                                                                           //
//    Project  : Ingres II/Vdba                                              //
//    Author   : UK Sotheavut                                                //
//                                                                           //
//                                                                           //
//    Purpose  : Index class for Data manipulation                  .        //
//                                                                           //
//  History after 04-May-1999:                                               //
//                                                                           //
//    06-Jan-2000 (schph01)                                                  //
//      BUG #99937                                                           //
//      Add comma when there are more than one location.                     //
******************************************************************************/
#include "stdafx.h"
#include "dmlindex.h"
#include "sqlkeywd.h"
#include "mainmfc.h"

extern "C"
{
#include "dba.h"
#include "tools.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaIndexStructure::CaIndexStructure(int nStructType, UINT idsStructName)
{

}

CaIndexStructure::CaIndexStructure(int nStructType, LPCTSTR strStructure)
{
	m_nStructType  = nStructType;
	m_strStructure = strStructure;
}

CaIndexStructure::~CaIndexStructure()
{
	while (!m_listComponent.IsEmpty())
		delete m_listComponent.RemoveHead();
}

void CaIndexStructure::AddComponent (LPCTSTR lpszComponent, CaComponentValueConstraint::ConstraintType cType, LPCTSTR lpszValue)
{
	ASSERT (cType != CaComponentValueConstraint::CONSTRAINT_SPIN);
	CaComponentValueConstraint* pComp = new CaComponentValueConstraint (lpszComponent, cType, lpszValue);
	m_listComponent.AddTail (pComp);
}
	
void CaIndexStructure::AddComponent (LPCTSTR lpszComponent, int nLower, int nUpper, LPCTSTR lpszValue )
{
	CaComponentValueConstraint* pComp = new CaComponentValueConstraint (lpszComponent, CaComponentValueConstraint::CONSTRAINT_SPIN, lpszValue);
	pComp->SetRange (nLower, nUpper);
	m_listComponent.AddTail (pComp);
}

void CaIndexStructure::AddComponent (LPCTSTR lpszComponent, int nLower, int nUpper, CaComponentValueConstraint::ConstraintType cType, LPCTSTR lpszValue)
{
	CaComponentValueConstraint* pComp = new CaComponentValueConstraint (lpszComponent, CaComponentValueConstraint::CONSTRAINT_EDITNUMBER, lpszValue);
	pComp->SetRange (nLower, nUpper);
	m_listComponent.AddTail (pComp);
}


//
// Object: Column of Index 
// ------------------------

IMPLEMENT_SERIAL (CaColumnIndex, CaColumn, 1)
void CaColumnIndex::Serialize (CArchive& ar)
{
	CaColumn::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_bKeyed;
		ar << m_nSort;
	}
	else
	{
		ar >> m_bKeyed;
		ar >> m_nSort;
	}
}


//
// Object: Index 
// -------------
IMPLEMENT_SERIAL (CaIndex, CaDBObject, 1)
CaIndex::CaIndex():CaDBObject() 
{
	m_bPersistence   = TRUE;
	m_bUnique        = FALSE;
	m_nUniqueScope   = -1;
	m_bCompressData = FALSE;
	m_bCompressKey = FALSE;

	m_strStructure   = _T("");
	m_strPageSize    = _T("");
	m_strMinPage     = _T("");
	m_strMaxPage     = _T("");
	m_strLeaffill    = _T("");
	m_strNonleaffill = _T("");
	m_strAllocation  = _T("");
	m_strExtend      = _T("");
	m_strFillfactor  = _T("");
	m_strMaxX = _T("");
	m_strMaxY = _T("");
	m_strMinX = _T("");
	m_strMinY = _T("");

	m_nStructure = 1;
	m_nPageSize  = 0;

}

CaIndex::CaIndex(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem)
	:CaDBObject(lpszItem, lpszOwner, bSystem) 
{
	m_bPersistence   = TRUE;
	m_bUnique        = FALSE;
	m_nUniqueScope   = -1;
	m_bCompressData  = FALSE;
	m_bCompressKey   = FALSE;

	m_strStructure   = _T("Isam");
	m_strPageSize    = _T("2048");
	m_strMinPage     = _T("");
	m_strMaxPage     = _T("");
	m_strLeaffill    = _T("");
	m_strNonleaffill = _T("");
	m_strAllocation  = _T("4");
	m_strExtend      = _T("16");
	m_strFillfactor  = _T("80");
	m_strMaxX = _T("");
	m_strMaxY = _T("");
	m_strMinX = _T("");
	m_strMinY = _T("");

	m_nStructure = 0; // Isam
	m_nPageSize  = 0; // 2K
}


CaIndex::~CaIndex()
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
}

void CaIndex::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	m_listColumn.Serialize (ar);
	
	if (ar.IsStoring())
	{
		// TODO: implement it
		ASSERT (FALSE);
	}
	else
	{
		// TODO: implement it
		ASSERT (FALSE);
	}
}

BOOL CaIndex::IsValide(CString& strMsg)
{
	if (m_strItem.IsEmpty())
	{
		strMsg = VDBA_MfcResourceString(IDS_E_SPEC_NAME_INDEX);//_T("You must specify the name of index");
		return FALSE;
	}

	if (m_strItem.FindOneOf (_T("<>")) != -1)
	{
		//_T("You must change index's name %s to the valid name")
		strMsg.Format (IDS_F_CHANGE_INDEX_NAME, (LPCTSTR)m_strItem);
		return FALSE;
	}

	if (m_listColumn.IsEmpty())
	{
		//_T("You must specify the columns of index %s")
		strMsg.Format (IDS_F_SPEC_COL_INDEX, (LPCTSTR)m_strItem);
		return FALSE;
	}

	if (m_strStructure.CompareNoCase (_T("RTree")) == 0 && m_listColumn.GetCount() > 1)
	{
		//_T("You can only specify one column of the TRee index %s")
		strMsg.Format (IDS_F_ONE_COL_INDEX, (LPCTSTR)m_strItem);
		return FALSE;
	}

	return TRUE;
}

BOOL CaIndex::GenerateWithClause(CString& strWithClause, CString& strListKey)
{
	strWithClause = _T("");
	//
	// Generate STRUCTURE:
	if (!m_strStructure.IsEmpty())
	{
		strWithClause += _T(" ");
		strWithClause += cstrStructure;
		strWithClause += _T(" = ");
		strWithClause += m_strStructure;
	}
	else
		return FALSE; // Structure must be specified in the actual design
	//
	// Generate KEY:
	if (!strListKey.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrKey;
		strWithClause += _T(" = (");
		strWithClause += strListKey;
		strWithClause += _T(")");
	}
	//
	// Generate FILLFACTOR:
	if (!m_strFillfactor.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrFillfactor;
		strWithClause += _T(" = ");
		strWithClause += m_strFillfactor;
	}
	//
	// Generate MINPAGE:
	if (!m_strMinPage.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrMinPage;
		strWithClause += _T(" = ");
		strWithClause += m_strMinPage;
	}
	//
	// Generate MAXPAGE:
	if (!m_strMaxPage.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrMaxPage;
		strWithClause += _T(" = ");
		strWithClause += m_strMaxPage;
	}
	//
	// Generate LEAFFILL:
	if (!m_strLeaffill.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrLeaffill;
		strWithClause += _T(" = ");
		strWithClause += m_strLeaffill;
	}
	//
	// Generate NONLEAFFILL:
	if (!m_strNonleaffill.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrNonleaffill;
		strWithClause += _T(" = ");
		strWithClause += m_strNonleaffill;
	}
	//
	// Generate the location:
	POSITION pos = m_listLocation.GetHeadPosition();
	if (pos != NULL)
	{
		BOOL bSingleLoc = TRUE;
		strWithClause += _T(", ");
		strWithClause += cstrLocation;
		strWithClause += _T(" = (");
		while (pos != NULL)
		{
			if (!bSingleLoc)
				strWithClause += _T(", ");
			strWithClause += QuoteIfNeeded(m_listLocation.GetNext (pos));
			bSingleLoc = FALSE;
		}
		strWithClause += _T(")");
	}
	//
	// Generate ALLOCATION:
	if (!m_strAllocation.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrAllocation;
		strWithClause += _T(" = ");
		strWithClause += m_strAllocation;
	}
	//
	// Generate EXTEND:
	if (!m_strExtend.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrExtend;
		strWithClause += _T(" = ");
		strWithClause += m_strExtend;
	}
	//
	// Generate COMPRESS: (actually ignore the keyword 'HI')
	// By default, index is not compressed.
	BOOL bCopress = FALSE;
	if (m_bCompressKey || m_bCompressData)
	{
		strWithClause += _T(", ");
		strWithClause += cstrCompression;
		strWithClause += _T(" = (");
		if (m_bCompressKey)
		{
			strWithClause += cstrKey;
			bCopress = TRUE;
		}

		if (m_bCompressData)
		{
			if (bCopress)
				strWithClause += _T(", ");
			strWithClause += cstrData;
		}
		strWithClause += _T(")");
	}
	//
	// Generate PERSISTENCE:
	if (m_bPersistence)
	{
		strWithClause += _T(", ");
		strWithClause += cstrPersistence;
	}
	else
	{
		strWithClause += _T(", ");
		strWithClause += cstrNo;
		strWithClause += cstrPersistence;
	}
	//
	// Generate UNIQUE SCOPE:
	if (m_bUnique)
	{
		strWithClause += _T(", ");
		strWithClause += cstrUniqueScope;
		strWithClause += _T(" = ");
		if (m_nUniqueScope == INDEX_UNIQUE_SCOPE_ROW)
			strWithClause += cstrRow;
		else
			strWithClause += cstrStatement;
	}
	//
	// Generate RANGE: (for TREE index only)
	if (m_strStructure.CompareNoCase (_T("RTree")) == 0)
	{
		CString strFormat;
		strWithClause += _T(", ");

		strFormat.Format (
			_T("%s = ((%s, %s), (%s, %s))"), 
			cstrRange, 
			(LPCTSTR)m_strMinX, 
			(LPCTSTR)m_strMinY, 
			(LPCTSTR)m_strMaxX, 
			(LPCTSTR)m_strMaxY);
		strWithClause += strFormat;
	}
	//
	// Generate PAGESIZE:
	if (!m_strPageSize.IsEmpty())
	{
		strWithClause += _T(", ");
		strWithClause += cstrPageSize;
		strWithClause += _T(" = ");
		strWithClause += m_strPageSize;
	}
	return TRUE;
}
