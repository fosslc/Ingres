/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdadoc.cpp, Implementation File  
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the VSDA Window.
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/


#include "stdafx.h"
#include "vsdadoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_SERIAL(CdSdaDoc, CDocument, 1)
CdSdaDoc::CdSdaDoc()
{
	m_strNode1 = _T("");
	m_strDatabase1 = _T("");
	m_strUser1 = _T("");
	m_strNode2 = _T("");
	m_strDatabase2 = _T("");
	m_strUser2 = _T("");

	m_nSchema1Type = -1; 
	m_nSchema2Type = -1; 
}


CdSdaDoc::~CdSdaDoc()
{
}


BEGIN_MESSAGE_MAP(CdSdaDoc, CDocument)
	//{{AFX_MSG_MAP(CdSdaDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdSdaDoc diagnostics

#ifdef _DEBUG
void CdSdaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdSdaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSdaDoc serialization

void CdSdaDoc::Serialize(CArchive& ar)
{
	ASSERT(FALSE);
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}