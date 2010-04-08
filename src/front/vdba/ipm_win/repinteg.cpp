/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : repevent.h, Header File
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : Philippe SCHALK
** Purpose  : The data for Replication : Integrity Object
** 
** History:
**
** xx-Jan-1998 (Philippe SCHALK)
**    Created.
*/

#include "stdafx.h"
#include "repinteg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// declaration for serialization
IMPLEMENT_SERIAL (CaReplicCommon					, CObject, 1)
IMPLEMENT_SERIAL (CaReplicCommonList				, CObject, 1)
IMPLEMENT_SERIAL (CaReplicIntegrityRegTable			, CObject, 1)
IMPLEMENT_SERIAL (CaReplicIntegrityRegTableList		, CObject, 1)

CaReplicIntegrityRegTable::CaReplicIntegrityRegTable(LPCTSTR lpszTbl_Name,LPCTSTR lpszOwner_name,
													 LPCTSTR lpszLookupTable_Name,LPCTSTR lpszDbName,
													 LPCTSTR lpszVnodeName,int iNodeHandle,int iRepVer,
													 int iTbl_no, int iCddsNo)
{
	m_strDbName		= lpszDbName;
	m_strVnodeName	= lpszVnodeName;
	m_nNodeHandle	= iNodeHandle;
	m_nReplicVersion = iRepVer;

	m_strTable_Name			= lpszTbl_Name;
	m_strOwner_name			= lpszOwner_name;
	m_strLookupTable_Name	= lpszLookupTable_Name;
	m_nTable_no				= iTbl_no;
	m_bValid				= TRUE;
	m_nCdds_No				= iCddsNo;
}

int CaReplicIntegrityRegTable::GetCdds_No()
{
	return m_nCdds_No;
}

CaReplicIntegrityRegTableList::~CaReplicIntegrityRegTableList()
{
	while (!IsEmpty())
		delete RemoveHead();
}

void CaReplicIntegrityRegTableList::ClearList()
{
	while (!IsEmpty())
		delete RemoveHead();
}

void CaReplicIntegrityRegTable::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nCdds_No; ;
		ar << m_strDbName;;
		ar << m_strVnodeName;
		ar << m_nNodeHandle;
		ar << m_nReplicVersion;
		ar << m_strTable_Name;
		ar << m_strOwner_name;
		ar << m_strLookupTable_Name;
		ar << m_nTable_no;
		ar << m_bValid;
	}
	else
	{
		ar >> m_nCdds_No; ;
		ar >> m_strDbName;;
		ar >> m_strVnodeName;
		ar >> m_nNodeHandle;
		ar >> m_nReplicVersion;
		ar >> m_strTable_Name;
		ar >> m_strOwner_name;
		ar >> m_strLookupTable_Name;
		ar >> m_nTable_no;
		ar >> m_bValid;
	}

}
CaReplicCommonList::~CaReplicCommonList()
{
	while (!IsEmpty())
		delete RemoveHead();
}
void CaReplicCommonList::ClearList()
{
	while (!IsEmpty())
		delete RemoveHead();
}
void CaReplicCommon::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strValue;
		ar << m_nNumberValue;
	}
	else
	{
		ar >> m_strValue;
		ar >> m_nNumberValue;
	}
}
