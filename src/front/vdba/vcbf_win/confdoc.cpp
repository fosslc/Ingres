/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : confdoc.cpph, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : App Main Doc                                                          //
****************************************************************************************/
#include "stdafx.h"
#include "vcbf.h"
#include "confdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfDoc

//IMPLEMENT_DYNCREATE(CConfDoc, CDocument)

CConfDoc::CConfDoc()
{
}

BOOL CConfDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CConfDoc::~CConfDoc()
{
}


BEGIN_MESSAGE_MAP(CConfDoc, CDocument)
	//{{AFX_MSG_MAP(CConfDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfDoc diagnostics

#ifdef _DEBUG
void CConfDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CConfDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CConfDoc serialization

void CConfDoc::Serialize(CArchive& ar)
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
// CConfDoc commands
