/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachedoc.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Document for Frame/View the page of DBMS Cache                        //
****************************************************************************************/
#include "stdafx.h"
#include "vcbf.h"
#include "cachedoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdCacheDoc

CdCacheDoc::CdCacheDoc()
{
}

BOOL CdCacheDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdCacheDoc::~CdCacheDoc()
{
}


BEGIN_MESSAGE_MAP(CdCacheDoc, CDocument)
	//{{AFX_MSG_MAP(CdCacheDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdCacheDoc diagnostics

#ifdef _DEBUG
void CdCacheDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdCacheDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdCacheDoc serialization

void CdCacheDoc::Serialize(CArchive& ar)
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
// CdCacheDoc commands
