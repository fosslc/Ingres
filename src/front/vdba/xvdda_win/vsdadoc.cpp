/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdadoc.cpp : implementation of the CdSda class
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame/View/Doc Architecture (doc)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "vsda.h"
#include "vsdadoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CdSda, CDocument)

BEGIN_MESSAGE_MAP(CdSda, CDocument)
	//{{AFX_MSG_MAP(CdSda)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdSda construction/destruction

CdSda::CdSda()
{
	// TODO: add one-time construction code here

}

CdSda::~CdSda()
{
}

BOOL CdSda::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CdSda serialization

void CdSda::Serialize(CArchive& ar)
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
// CdSda diagnostics

#ifdef _DEBUG
void CdSda::AssertValid() const
{
	CDocument::AssertValid();
}

void CdSda::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSda commands
