/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : maindoc.cpp, Implementation File                                      //
//                                                                                     //
//                                                                                     //
//    Project  : Virtual Node Manager.                                                 //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Main Document (Frame, View, Doc design). It contains the Tree Data    //
****************************************************************************************/

#include "stdafx.h"
#include "vnodemgr.h"
#include "maindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc

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
	CString strTitle;
	strTitle.LoadString (AFX_IDS_APP_TITLE);
	SetTitle (strTitle);
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
