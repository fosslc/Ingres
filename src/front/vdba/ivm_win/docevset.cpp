/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : docevset.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Document for Event Setting Frame                                      //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "docevset.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdEventSetting

IMPLEMENT_DYNCREATE(CdEventSetting, CDocument)

CdEventSetting::CdEventSetting()
{
}

BOOL CdEventSetting::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdEventSetting::~CdEventSetting()
{
}


BEGIN_MESSAGE_MAP(CdEventSetting, CDocument)
	//{{AFX_MSG_MAP(CdEventSetting)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdEventSetting diagnostics

#ifdef _DEBUG
void CdEventSetting::AssertValid() const
{
	CDocument::AssertValid();
}

void CdEventSetting::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdEventSetting serialization

void CdEventSetting::Serialize(CArchive& ar)
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
// CdEventSetting commands
