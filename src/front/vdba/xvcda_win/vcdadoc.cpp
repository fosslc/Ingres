/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdadoc.cpp : implementation of the CdCda class
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : frame/view/doc architecture (doc)
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
**/

#include "stdafx.h"
#include "vcda.h"
#include "vcdadoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdCda

IMPLEMENT_DYNCREATE(CdCda, CDocument)

BEGIN_MESSAGE_MAP(CdCda, CDocument)
	//{{AFX_MSG_MAP(CdCda)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdCda construction/destruction

CdCda::CdCda()
{
	// TODO: add one-time construction code here

}

CdCda::~CdCda()
{
}

BOOL CdCda::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CdCda serialization

void CdCda::Serialize(CArchive& ar)
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
// CdCda diagnostics

#ifdef _DEBUG
void CdCda::AssertValid() const
{
	CDocument::AssertValid();
}

void CdCda::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdCda commands
