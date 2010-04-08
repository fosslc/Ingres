/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : maindoc.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "ija.h"
#include "maindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CdMainDoc, CDocument)

BEGIN_MESSAGE_MAP(CdMainDoc, CDocument)
	//{{AFX_MSG_MAP(CdMainDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc construction/destruction

CdMainDoc::CdMainDoc()
{
	switch (theApp.m_cmdLine.m_nArgs)
	{
	case 0:
		m_strFormat = _T("");
		break;
	case 1:
		m_strFormat.Format(_T(" - %s"), (LPCTSTR)theApp.m_cmdLine.m_strNode);
		break;
	case 2:
		m_strFormat.Format(_T(" - %s::%s"), 
		    (LPCTSTR)theApp.m_cmdLine.m_strNode,
		    (LPCTSTR)theApp.m_cmdLine.m_strDatabase);
		break;
	case 3:
		m_strFormat.Format(_T(" - %s::%s %s.%s"), 
		    (LPCTSTR)theApp.m_cmdLine.m_strNode,
		    (LPCTSTR)theApp.m_cmdLine.m_strDatabase,
		    (LPCTSTR)theApp.m_cmdLine.m_strTableOwner,
		    (LPCTSTR)theApp.m_cmdLine.m_strTable);
		break;
	}
}

CdMainDoc::~CdMainDoc()
{
}

BOOL CdMainDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CdMainDoc serialization

void CdMainDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc diagnostics

#ifdef _DEBUG
void CdMainDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdMainDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc commands
