/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dcdtdiff.cpp : implementation file
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the property page (Frame/View/Doc)
**               (CfDifferenceDetail/CvDifferenceDetail)
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#include "stdafx.h"
#include "vsda.h"
#include "dcdtdiff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL(CdDifferenceDetail, CDocument, 1)
CdDifferenceDetail::CdDifferenceDetail()
{
}

BOOL CdDifferenceDetail::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdDifferenceDetail::~CdDifferenceDetail()
{
}


BEGIN_MESSAGE_MAP(CdDifferenceDetail, CDocument)
	//{{AFX_MSG_MAP(CdDifferenceDetail)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdDifferenceDetail diagnostics

#ifdef _DEBUG
void CdDifferenceDetail::AssertValid() const
{
	CDocument::AssertValid();
}

void CdDifferenceDetail::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdDifferenceDetail serialization

void CdDifferenceDetail::Serialize(CArchive& ar)
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
// CdDifferenceDetail commands
