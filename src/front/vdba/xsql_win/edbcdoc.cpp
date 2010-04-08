/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edbcdoc.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : interface of the CdSqlQueryEdbc class
**
** History:
**
** 09-Oct-2003 (uk$so01)
**    SIR #106648, (EDBC Specific).
*/

#include "stdafx.h"
#include "xsql.h"
#include "edbcdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryEdbc

IMPLEMENT_DYNCREATE(CdSqlQueryEdbc, CdSqlQuery)

CdSqlQueryEdbc::CdSqlQueryEdbc():CdSqlQuery()
{
}

BOOL CdSqlQueryEdbc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdSqlQueryEdbc::~CdSqlQueryEdbc()
{
}

BEGIN_MESSAGE_MAP(CdSqlQueryEdbc, CdSqlQuery)
	//{{AFX_MSG_MAP(CdSqlQueryEdbc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryEdbc diagnostics

#ifdef _DEBUG
void CdSqlQueryEdbc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdSqlQueryEdbc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryEdbc serialization

void CdSqlQueryEdbc::Serialize(CArchive& ar)
{
	CdSqlQuery::Serialize(ar);
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryEdbc commands

