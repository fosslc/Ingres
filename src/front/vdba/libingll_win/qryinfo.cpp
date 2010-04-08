/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : qryinfo.cpp: Implementation for the CaLLAddAlterDrop, 
**               CaLLQueryInfo class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access low lewel data (Ingres Database)
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 22-Apr-2003 (schph01)
**    SIR 107523 Add sequence Object
** 29-Sep-2004 (uk$so01)
**    BUG #113119, Add readlock mode in the session management
** 21-Oct-2004 (uk$so01)
**    BUG #113280 / ISSUE 13742473 (VDDA should minimize the number of DBMS connections)
**/


#include "stdafx.h"
#include "qryinfo.h"
#include "ingobdml.h"
#include "constdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// CLASS: CaConnectInfo
// ************************************************************************************************
IMPLEMENT_SERIAL (CaConnectInfo, CObject, 1)
CaConnectInfo::CaConnectInfo()
{
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_strDatabaseOwner = _T("");
	m_strServerClass = _T("");
	m_strOptions = _T("");
	m_nFlag = 0;
	m_bIndependent = FALSE;
	m_strUser = _T("");
	m_nSessionLock = 0;
}


CaConnectInfo::CaConnectInfo(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDBOwner)
{
	m_strNode = lpszNode;
	m_strDatabase = lpszDatabase;
	m_strDatabaseOwner = lpszDBOwner;
	m_strServerClass = _T("");
	m_strOptions = _T("");
	m_nFlag = 0;
	m_bIndependent = FALSE;
	m_strUser = _T("");
	m_nSessionLock = 0;
}

CaConnectInfo::CaConnectInfo(const CaConnectInfo& c)
{
	Copy(c);
}

CaConnectInfo CaConnectInfo::operator = (const CaConnectInfo& c)
{
	Copy (c);
	return *this;
}

BOOL CaConnectInfo::Matched (const CaConnectInfo& c)
{
	if (m_nFlag != c.GetFlag())
		return FALSE;
	if (m_strNode.CompareNoCase(c.GetNode()) != 0)
		return FALSE;
	if (m_strDatabase.CompareNoCase(c.GetDatabase()) != 0)
		return FALSE;
	if (m_strUser.CompareNoCase(c.GetUser()) != 0)
		return FALSE;
	if (m_strServerClass.CompareNoCase(c.GetServerClass()) != 0)
		return FALSE;
	if (m_bIndependent != c.GetIndependent())
		return FALSE;
	if (m_strOptions.CompareNoCase(c.GetOptions()) != 0)
		return FALSE;
	if (m_nSessionLock != c.m_nSessionLock)
		return FALSE;
	return TRUE;
}

void CaConnectInfo::Copy (const CaConnectInfo& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	m_strNode  = c.m_strNode;
	m_strDatabase = c.m_strDatabase;
	m_strDatabaseOwner = c.m_strDatabaseOwner;
	m_strServerClass = c.m_strServerClass;
	m_strOptions = c.m_strOptions;
	m_nFlag = c.m_nFlag;
	m_strUser = c.m_strUser;
	m_bIndependent = c.m_bIndependent;
	m_nSessionLock = c.m_nSessionLock;
}


void CaConnectInfo::Serialize (CArchive& ar)
{
	CObject::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_strNode;
		ar << m_strDatabase;
		ar << m_strDatabaseOwner;
		ar << m_strServerClass;
		ar << m_strOptions;
		ar << m_nFlag;
		ar << m_bIndependent;
		ar << m_strUser;
		ar << m_nSessionLock;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_strNode;
		ar >> m_strDatabase;
		ar >> m_strDatabaseOwner;
		ar >> m_strServerClass;
		ar >> m_strOptions;
		ar >> m_nFlag;
		ar >> m_bIndependent;
		ar >> m_strUser;
		ar >> m_nSessionLock;
	}
}




//
// CLASS: CaLLQueryInfo
// ************************************************************************************************
IMPLEMENT_SERIAL (CaLLQueryInfo, CaConnectInfo, 1)
CaLLQueryInfo::CaLLQueryInfo(): CaConnectInfo()
{
	Initialize();
}

CaLLQueryInfo::CaLLQueryInfo(int nObjType, LPCTSTR lpszNode, LPCTSTR lpszDatabase):
	CaConnectInfo(lpszNode, lpszDatabase, _T(""))
{
	Initialize();
	m_nTypeObject = nObjType;
}

CaLLQueryInfo::CaLLQueryInfo(const CaLLQueryInfo& c)
{
	Copy(c);
}

CaLLQueryInfo CaLLQueryInfo::operator = (const CaLLQueryInfo& c)
{
	Copy (c);
	return *this;
}

void CaLLQueryInfo::Copy (const CaLLQueryInfo& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	CaConnectInfo::Copy(c);

	m_nFetch = c.m_nFetch;
	m_nTypeObject = c.m_nTypeObject;
	m_nSubObjectType = c.m_nSubObjectType;
	m_strItem2 = c.m_strItem2;
	m_strItem2Owner = c.m_strItem2Owner;
	m_strItem3 = c.m_strItem3;
	m_strItem3Owner = c.m_strItem3Owner;
}

void CaLLQueryInfo::Initialize ()
{
	m_nFetch = FETCH_ALL;
	m_nTypeObject = -1;
	m_nSubObjectType = -1;
	m_strItem2 = _T("");
	m_strItem2Owner = _T(""); 
	m_strItem3 = _T("");
	m_strItem3Owner = _T("");
}



void CaLLQueryInfo::Serialize (CArchive& ar)
{
	CaConnectInfo::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_nTypeObject;
		ar << m_strItem2;
		ar << m_strItem2Owner;
		ar << m_strItem3;
		ar << m_strItem3Owner;
		ar << m_nFetch;
		ar << m_nSubObjectType;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_nTypeObject;
		ar >> m_strItem2;
		ar >> m_strItem2Owner;
		ar >> m_strItem3;
		ar >> m_strItem3Owner;
		ar >> m_nFetch;
		ar >> m_nSubObjectType;
	}
}


CString CaLLQueryInfo::GetConnectionString()
{
	CString strConnection = _T("");
	switch (m_nTypeObject)
	{
	case OBT_DATABASE:
	case OBT_PROFILE:
	case OBT_USER:
	case OBT_GROUP:

	//case OBT_GROUPUSER:

	case OBT_ROLE:
	case OBT_LOCATION:
	//case OBT_DBAREA:   // OI Desktop
	//case OBT_STOGROUP: // OI Desktop
		ASSERT (!m_strNode.IsEmpty());
		if (m_strNode.IsEmpty())
			return strConnection;
		strConnection.Format (_T("%s::%s"), (LPCTSTR)m_strNode, _T("iidbdb"));
		break;

	case OBT_TABLE:
	case OBT_VIEW:
	case OBT_PROCEDURE:
	case OBT_SEQUENCE:
	case OBT_DBEVENT:
	case OBT_SYNONYM:
		ASSERT (!m_strNode.IsEmpty());
		if (m_strNode.IsEmpty())
			return strConnection;
		ASSERT (!GetItem1().IsEmpty());
		if (GetItem1().IsEmpty())
			return strConnection;
		strConnection.Format (_T("%s::%s"), (LPCTSTR)m_strNode, (LPCTSTR)GetItem1());
		break;
	case OBT_RULE:
	case OBT_INDEX:
	case OBT_TABLECOLUMN:
	case OBT_VIEWCOLUMN:
	case OBT_INTEGRITY:
		ASSERT (!m_strNode.IsEmpty());
		if (m_strNode.IsEmpty())
			return strConnection;
		ASSERT (!GetItem1().IsEmpty());
		if (GetItem1().IsEmpty())
			return strConnection;
		ASSERT (!m_strItem2.IsEmpty() && !m_strItem2Owner.IsEmpty());
		if (m_strItem2.IsEmpty() || m_strItem2.IsEmpty())
			return strConnection;
		strConnection.Format (_T("%s::%s"), (LPCTSTR)m_strNode, GetItem1());
		break;

	default:
		ASSERT (FALSE);
		break;
	}
	return strConnection;
}


//
// CLASS: CaLLAddAlterDrop
// ************************************************************************************************
IMPLEMENT_SERIAL (CaLLAddAlterDrop, CaConnectInfo, 1)
void CaLLAddAlterDrop::Serialize (CArchive& ar)
{
	CaConnectInfo::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_strStatement;
	}
	else
	{
		ar >> m_strStatement;
	}
}


BOOL CaLLAddAlterDrop::ExecuteStatement(CaSessionManager* pSessionManager)
{
	return INGRESII_llExecuteImmediate (this, pSessionManager);
}