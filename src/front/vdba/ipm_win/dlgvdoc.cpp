/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dlgvdoc.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document for CFormView, Scrollable Dialog box of Detail Page
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "dlgvdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgViewDoc, CDocument)

CuDlgViewDoc::CuDlgViewDoc()
{
}

BOOL CuDlgViewDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CuDlgViewDoc::~CuDlgViewDoc()
{
}


BEGIN_MESSAGE_MAP(CuDlgViewDoc, CDocument)
	//{{AFX_MSG_MAP(CuDlgViewDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgViewDoc diagnostics

#ifdef _DEBUG
void CuDlgViewDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CuDlgViewDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgViewDoc serialization

void CuDlgViewDoc::Serialize(CArchive& ar)
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
// CuDlgViewDoc commands
