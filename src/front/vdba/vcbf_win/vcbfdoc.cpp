/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : vcbfdoc.cpp, Implementation file                                      //
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

#include "vcbfdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVcbfDoc

IMPLEMENT_DYNCREATE(CVcbfDoc, CDocument)

BEGIN_MESSAGE_MAP(CVcbfDoc, CDocument)
	//{{AFX_MSG_MAP(CVcbfDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVcbfDoc construction/destruction

CVcbfDoc::CVcbfDoc()
{
	// TODO: add one-time construction code here

}

CVcbfDoc::~CVcbfDoc()
{
}

BOOL CVcbfDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVcbfDoc serialization

void CVcbfDoc::Serialize(CArchive& ar)
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
// CVcbfDoc diagnostics

#ifdef _DEBUG
void CVcbfDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVcbfDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVcbfDoc commands
