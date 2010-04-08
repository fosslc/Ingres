/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlalarm.cpp: implementation of the CaAlarm class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Alarm object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#include "stdafx.h"
#include "dmlalarm.h"
#include "cmdargs.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Alarm
// ************************************************************************************************
IMPLEMENT_SERIAL (CaAlarm, CaDBObject, 1)
CaAlarm::CaAlarm(): CaDBObject()
{
	Init();
}

CaAlarm::CaAlarm(LPCTSTR lpszName, int nNumber)
    : CaDBObject(lpszName, _T(""))
{
	Init();
	m_nSecurityNumber = nNumber;
}

void CaAlarm::Init()
{
	SetObjectID(OBT_ALARM);
	m_bParesed = FALSE;
	m_nSecurityNumber = -1;
	m_tchSubjectType  = _T('\0');
	m_strSecurityUser = _T("");
	m_strDBEvent = _T("");
	m_strDBEventOwner = _T("");
	m_strDetailText  = _T("");
	if (!m_listEventAction.IsEmpty())
		m_listEventAction.RemoveAll();
	m_bIfSuccess = FALSE;
	m_bIfFailure = FALSE;
}

CaAlarm::CaAlarm(const CaAlarm& c)
{
	Copy (c);
}

CaAlarm CaAlarm::operator = (const CaAlarm& c)
{
	Copy (c);
	return *this;
}

void CaAlarm::Copy (const CaAlarm& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	Init();
	CaDBObject::Copy(c);

	m_bParesed = c.m_bParesed;
	m_nSecurityNumber = c.m_nSecurityNumber;
	m_tchSubjectType = c.m_tchSubjectType;
	m_strSecurityUser = c.m_strSecurityUser;
	m_strDBEvent = c.m_strDBEvent;
	m_strDBEventOwner = c.m_strDBEventOwner;
	m_strDetailText = c.m_strDetailText;
	m_bIfSuccess = c.m_bIfSuccess;
	m_bIfFailure = c.m_bIfFailure;
	CStringList& lev = ((CaAlarm*)&c)->GetListEventAction();
	POSITION pos = lev.GetHeadPosition();
	while (pos != NULL)
	{
		CString strEv = lev.GetNext(pos);
		m_listEventAction.AddTail(strEv);
	}
}

void CaAlarm::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	m_listEventAction.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_bParesed;
		ar << m_nSecurityNumber;
		ar << m_tchSubjectType;
		ar << m_strSecurityUser;
		ar << m_strDBEvent;
		ar << m_strDBEventOwner;
		ar << m_strDetailText;
		ar << m_bIfSuccess;
		ar << m_bIfFailure;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_bParesed;
		ar >> m_nSecurityNumber;
		ar >> m_tchSubjectType;
		ar >> m_strSecurityUser;
		ar >> m_strDBEvent;
		ar >> m_strDBEventOwner;
		ar >> m_strDetailText;
		ar >> m_bIfSuccess;
		ar >> m_bIfFailure;
	}
}

BOOL CaAlarm::Matched (CaAlarm* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME || nFlag == MATCHED_NAMExOWNER)
	{
		return (pObj->GetName().CompareNoCase (m_strItem) == 0);
	}
	else
	if (nFlag == MATCHED_ALL)
	{
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

void CaAlarm::HandleToken(CString& strToken)
{
	if (strToken[0] == _T('\'') || strToken[0] == _T('\"'))
		return;
	if (strToken.Find(_T("success")) != -1)
		m_bIfSuccess = TRUE;
	if (strToken.Find(_T("failure")) != -1)
		m_bIfFailure = TRUE;

	if (strToken.GetLength() < 6)
		return;
	if (strToken.GetLength() > 10)
		return;
	if (strToken.Find(_T("select")) == 0)
		m_listEventAction.AddTail(_T("select"));
	else
	if (strToken.Find(_T("delete")) == 0)
		m_listEventAction.AddTail(_T("delete"));
	else
	if (strToken.Find(_T("insert")) == 0)
		m_listEventAction.AddTail(_T("insert"));
	else
	if (strToken.Find(_T("update")) == 0)
		m_listEventAction.AddTail(_T("update"));
	else
	if (strToken.Find(_T("connect")) == 0)
		m_listEventAction.AddTail(_T("connect"));
	else
	if (strToken.Find(_T("disconnect")) == 0)
		m_listEventAction.AddTail(_T("disconnect"));
}

BOOL CaAlarm::ParseSecurityAlarmFlags()
{
	CStringArray arrayToken;
	CaArgumentLine::MakeListTokens(m_strDetailText, arrayToken, _T(","));
	int i = 0, nSize = arrayToken.GetSize();
	while (i<nSize)
	{
		CString strToken = arrayToken.GetAt(i);
		HandleToken(strToken);
		i++;
	}

	AlreadyParsed(TRUE);
	return TRUE;
}

CString CaAlarm::GetSecurityUser()
{
	if (m_tchSubjectType == _T('P'))
		return _T("(public)");
	else
		return m_strSecurityUser;
}
