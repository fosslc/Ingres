/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tbstatis.cpp, Implementation File 
**    Project  : CA-OpenIngres/Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The Statistics of the table 
**
** History after 3 may 2001
** 03-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 08-Oct-2001 (schph01)
**    SIR #105881
**    Manage variable member m_nNumColumnKey in CaTableStatistic class
**
*/


#include "stdafx.h"
#include "tbstatis.h"
#include "calsctrl.h" // SORTPARAMS structure

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CaTableStatisticColumn
// ----------------------
IMPLEMENT_SERIAL (CaTableStatisticColumn, CObject, 2)

CaTableStatisticColumn::CaTableStatisticColumn()
	:m_strColumn(_T("")), m_strType(_T("")), m_strDefault (_T(""))
{
	m_bHasStatistic = FALSE;
	m_nLength       = 0;
	m_nPKey         = 0;
	m_bNullable     = FALSE;
	m_bWithDefault  = FALSE;
	m_nScale        = -1;
	m_bComposite    = FALSE;
}

CaTableStatisticColumn::CaTableStatisticColumn(const CaTableStatisticColumn& c)
{
	Copy (c);
}


CaTableStatisticColumn CaTableStatisticColumn::operator =(const CaTableStatisticColumn& c)
{
	ASSERT (&c != this);
	Copy (c);
	return *this;
}

void CaTableStatisticColumn::Copy(const CaTableStatisticColumn& c)
{
	m_strColumn     = c.m_strColumn;
	m_strType       = c.m_strType;
	m_strDefault    = c.m_strDefault;
	m_bHasStatistic = c.m_bHasStatistic;
	m_nLength       = c.m_nLength;
	m_nPKey         = c.m_nPKey;
	m_bNullable     = c.m_bNullable;
	m_bWithDefault  = c.m_bWithDefault;
	m_nScale        = c.m_nScale;
	m_bComposite    = c.m_bComposite;
}

void CaTableStatisticColumn::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strColumn;
		ar << m_bHasStatistic;
		ar << m_strType;
		ar << m_strDefault;
		ar << m_nLength;
		ar << m_nPKey;
		ar << m_bNullable;
		ar << m_bWithDefault;
		ar << m_nScale;
		ar << m_nDataType;
		ar << m_bComposite;
	}
	else
	{
		ar >> m_strColumn;
		ar >> m_bHasStatistic;
		ar >> m_strType;
		ar >> m_strDefault;
		ar >> m_nLength;
		ar >> m_nPKey;
		ar >> m_bNullable;
		ar >> m_bWithDefault;
		ar >> m_nScale;
		ar >> m_nDataType;
		ar >> m_bComposite;
	}
}


int CALLBACK CaTableStatisticColumn::Compare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SORTPARAMS* pSort = (SORTPARAMS*)lParamSort;
	ASSERT (pSort);
	CString strType1;
	CString strType2;
	int  nCode= 0;
	BOOL bAsc = pSort->m_bAsc;
	CaTableStatisticColumn* o1 = (CaTableStatisticColumn*) lParam1;
	CaTableStatisticColumn* o2 = (CaTableStatisticColumn*) lParam2;
	switch (pSort->m_nItem)
	{
	case COLUMN_STATISTIC:
		nCode = (o1->m_bHasStatistic > o2->m_bHasStatistic)? 1: (o1->m_bHasStatistic < o2->m_bHasStatistic)? -1: 0;
		break;
	case COLUMN_NAME:
		nCode = nCode = o1->m_strColumn.CompareNoCase (o2->m_strColumn);
		break;
	case COLUMN_TYPE:
		strType1.Format (_T("%s (%05d)"), (LPCTSTR)o1->m_strType, o1->m_nLength);
		strType2.Format (_T("%s (%05d)"), (LPCTSTR)o2->m_strType, o2->m_nLength);
		nCode = nCode = strType1.CompareNoCase (strType2);
		break;
	case COLUMN_PKEY:
		nCode = (o1->m_nPKey > o2->m_nPKey)? 1: (o1->m_nPKey < o2->m_nPKey)? -1: 0;
		break;
	case COLUMN_NULLABLE:
		nCode = (o1->m_bNullable > o2->m_bNullable)? 1: (o1->m_bNullable < o2->m_bNullable)? -1: 0;
		break;
	case COLUMN_WITHDEFAULT:
		nCode = (o1->m_bWithDefault > o2->m_bWithDefault)? 1: (o1->m_bWithDefault < o2->m_bWithDefault)? -1: 0;
		break;
	case COLUMN_DEFAULT:
		nCode = nCode = o1->m_strDefault.CompareNoCase (o2->m_strDefault);
		break;
	default:
		break;
	}
	return bAsc? nCode: -nCode;
}



// CaTableStatisticItem
// ----------------------
IMPLEMENT_SERIAL (CaTableStatisticItem, CObject, 1)
CaTableStatisticItem::CaTableStatisticItem(const CaTableStatisticItem& c)
{
	m_strValue    = c.m_strValue;
	m_dPercentage = c.m_dPercentage;
	m_dRepetitionFactor = c.m_dRepetitionFactor;
}


CaTableStatisticItem CaTableStatisticItem::operator =(const CaTableStatisticItem& c)
{
	ASSERT (&c != this);
	m_strValue    = c.m_strValue;
	m_dPercentage = c.m_dPercentage;
	m_dRepetitionFactor = c.m_dRepetitionFactor;
	return *this;
}


void CaTableStatisticItem::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strValue;
		ar << m_dPercentage;
		ar << m_dRepetitionFactor;
	}
	else
	{
		ar >> m_strValue;
		ar >> m_dPercentage;
		ar >> m_dRepetitionFactor;
	}
}



// CaTableStatistic
// ----------------------
IMPLEMENT_SERIAL (CaTableStatistic, CObject, 3)

CaTableStatistic::CaTableStatistic()
	:m_strVNode(_T("")), m_strDBName(_T("")), m_strTable(_T("")), m_strTableOwner(_T(""))
{
	m_lUniqueValue = 0l;
	m_lRepetitionFlag = 0l;
	m_bUniqueFlag = FALSE;
	m_bCompleteFlag = FALSE;
	m_bAsc = FALSE;
	m_nCurrentSort = -1;
	m_strIndex = _T("");
	m_nOT = -1;
	m_AddCompositeForHistogram = TRUE;
	m_nNumColumnKey = 0;
}

CaTableStatistic::CaTableStatistic(const CaTableStatistic& c)
{
	Copy (c);
}


CaTableStatistic CaTableStatistic::operator = (const CaTableStatistic& c)
{
	ASSERT (&c != this);
	Cleanup();
	Copy (c);
	return *this;
}

void CaTableStatistic::Copy (const CaTableStatistic& c)
{
	m_strVNode      = c.m_strVNode;
	m_strDBName     = c.m_strDBName;
	m_strTable      = c.m_strTable;
	m_strTableOwner = c.m_strTableOwner;
	m_lUniqueValue    = c.m_lUniqueValue;
	m_lRepetitionFlag = c.m_lRepetitionFlag;
	m_bUniqueFlag     = c.m_bUniqueFlag;
	m_bCompleteFlag   = c.m_bCompleteFlag;
	m_bAsc            = c.m_bAsc;
	m_nCurrentSort    = c.m_nCurrentSort;
	m_strIndex        = c.m_strIndex;
	m_nOT             = c.m_nOT;
	m_AddCompositeForHistogram = c.m_AddCompositeForHistogram;
	m_nNumColumnKey   = c.m_nNumColumnKey;

	CaTableStatisticColumn* pColumn = NULL;
	CaTableStatisticColumn* pNewColumn = NULL;
	POSITION pos = c.m_listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		pColumn = c.m_listColumn.GetNext(pos);
		pNewColumn = new CaTableStatisticColumn();
		*pNewColumn= *pColumn;
		m_listColumn.AddTail(pNewColumn);
	}
	
	CaTableStatisticItem* pItem = NULL;
	CaTableStatisticItem* pNewItem = NULL;
	pos = c.m_listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = c.m_listItem.GetNext(pos);
		pNewItem = new CaTableStatisticItem();
		*pNewItem= *pItem;
		m_listItem.AddTail(pNewItem);
	}
}


CaTableStatistic::~CaTableStatistic()
{
	Cleanup();
}

void CaTableStatistic::Cleanup()
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	while (!m_listItem.IsEmpty())
		delete m_listItem.RemoveHead();
}

void CaTableStatistic::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strVNode;
		ar << m_strDBName;
		ar << m_strTable;
		ar << m_strTableOwner;
		ar << m_lUniqueValue;
		ar << m_lRepetitionFlag;
		ar << m_bUniqueFlag;
		ar << m_bCompleteFlag;
		ar << m_nCurrentSort;
		ar << m_bAsc;
		ar << m_strIndex;
		ar << m_nOT;
		ar << m_AddCompositeForHistogram;
		ar << m_nNumColumnKey;
	}
	else
	{
		ar >> m_strVNode;
		ar >> m_strDBName;
		ar >> m_strTable;
		ar >> m_strTableOwner;
		ar >> m_lUniqueValue;
		ar >> m_lRepetitionFlag;
		ar >> m_bUniqueFlag;
		ar >> m_bCompleteFlag;
		ar >> m_nCurrentSort;
		ar >> m_bAsc;
		ar >> m_strIndex;
		ar >> m_nOT;
		ar >> m_AddCompositeForHistogram;
		ar >> m_nNumColumnKey;
	}
	m_listColumn.Serialize (ar);
	m_listItem.Serialize (ar);
}


void CaTableStatistic::MaxValues (double& maxPercent, double& maxRepFactor)
{
	maxPercent   = 0.0;
	maxRepFactor = 0.0;
	CaTableStatisticItem* pData = NULL;
	POSITION pos = m_listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pData = m_listItem.GetNext (pos);
		if (pData->m_dPercentage > maxPercent)
			maxPercent = pData->m_dPercentage;
		if (pData->m_dRepetitionFactor > maxRepFactor)
			maxRepFactor = pData->m_dRepetitionFactor;
	}
}
