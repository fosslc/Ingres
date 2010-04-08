/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : piedoc.cpp, Implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document for Drawing the statistic pie
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
*/


#include "stdafx.h"
#include "piedoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CdStatisticPieChartDoc, CDocument)
CdStatisticPieChartDoc::CdStatisticPieChartDoc()
{
	m_pPieInfo = NULL;
	m_pBarInfo = NULL;
}

void CdStatisticPieChartDoc::SetPieInfo (CaPieInfoData* pPie)
{
	if (m_pPieInfo) 
		delete m_pPieInfo;
	m_pPieInfo = pPie;
}


BOOL CdStatisticPieChartDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdStatisticPieChartDoc::~CdStatisticPieChartDoc()
{
	if (m_pPieInfo)
		delete m_pPieInfo;
	if (m_pBarInfo)
		delete m_pBarInfo;
}

void CdStatisticPieChartDoc::SetDiskInfo (CaBarInfoData* pBarInfo)
{
	if (m_pBarInfo)
		delete m_pBarInfo;
	m_pBarInfo = pBarInfo;
}

BEGIN_MESSAGE_MAP(CdStatisticPieChartDoc, CDocument)
	//{{AFX_MSG_MAP(CdStatisticPieChartDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdStatisticPieChartDoc diagnostics

#ifdef _DEBUG
void CdStatisticPieChartDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdStatisticPieChartDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdStatisticPieChartDoc serialization

void CdStatisticPieChartDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_pPieInfo;
		ar << m_pBarInfo;
	}
	else
	{
		ar >> m_pPieInfo;
		ar >> m_pBarInfo;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdStatisticPieChartDoc commands
