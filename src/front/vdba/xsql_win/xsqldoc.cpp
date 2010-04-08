/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xsqldoc.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : implementation of the CdSqlQuery class
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 01-Sept-2004 (schph01)
**    SIR #106648 initialize files names for the 'History of SQL Statements
**    Errors' dialog.
*/


#include "stdafx.h"
#include "xsql.h"
#include "xsqldoc.h"
#include "xsqlview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CdSqlQuery, CDocument)

BEGIN_MESSAGE_MAP(CdSqlQuery, CDocument)
	//{{AFX_MSG_MAP(CdSqlQuery)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CdSqlQuery::CdSqlQuery()
{
	m_strNode = _T("");
	m_strServer = _T("");
	m_strUser = GetDefaultConnectedUser();
	m_strDatabase = _T("");
	m_bConnected = FALSE;
	m_strConnectString = _T("");

	m_strWorkingFile = _T("");
	m_nDbFlag = 0;
	m_FilesErrors.InitializeFilesNames();
#if defined (_OPTION_GROUPxROLE)
	m_strGroup = GetDefaultConnectedGroup();
	m_strRole  = GetDefaultConnectedRole();
	m_strRolePassword = _T("");
	m_bRolePassword = FALSE;
#endif
}

CdSqlQuery::~CdSqlQuery()
{
}

void CdSqlQuery::SetNode(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strNode = lpszItem;
}

void CdSqlQuery::SetServer(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strServer = lpszItem;
}

void CdSqlQuery::SetUser(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strUser = lpszItem;
}

#if defined (_OPTION_GROUPxROLE)
void CdSqlQuery::SetGroup(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strGroup = lpszItem;
}

void CdSqlQuery::SetRole(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strRole = lpszItem;
}
#endif

void CdSqlQuery::SetDatabase(LPCTSTR lpszItem, UINT nFlag)
{
	m_bConnected=FALSE; 
	m_strDatabase = lpszItem;
	m_nDbFlag = nFlag;
}

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery serialization

void CdSqlQuery::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strNode;
		ar << m_strServer;
		ar << m_strUser;
		ar << m_strDatabase;
		ar << m_strWorkingFile;
		ar << m_nDbFlag;
		ar << m_strConnectString;
	}
	else
	{
		ar >> m_strNode;
		ar >> m_strServer;
		ar >> m_strUser;
		ar >> m_strDatabase;
		ar >> m_strWorkingFile;
		ar >> m_nDbFlag;
		ar >> m_strConnectString;
	}

#if defined (_OPTION_GROUPxROLE)
	if (ar.IsStoring())
	{
		ar << m_strGroup;
		ar << m_strRole;
		ar << m_strRolePassword;
		ar << m_bRolePassword;
	}
	else
	{
		ar >> m_strGroup;
		ar >> m_strRole;
		ar >> m_strRolePassword;
		ar >> m_bRolePassword;
	}
#endif

}

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery diagnostics

#ifdef _DEBUG
void CdSqlQuery::AssertValid() const
{
	CDocument::AssertValid();
}

void CdSqlQuery::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery commands
CuSqlQueryControl* CdSqlQuery::GetSqlQueryCtrl()
{
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		if (pView->IsKindOf(RUNTIME_CLASS(CvSqlQuery)))
		{
			CvSqlQuery* pSqlView = (CvSqlQuery* )pView;
			return &pSqlView->m_SqlQuery;
		}
	}
	return NULL;
}
