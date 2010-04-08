/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsqlview.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : implementation of the CvSqlQuery class
**
** History:
**
** 18-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate SQL Test ActiveX Control
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qsqldoc.h"
#include "qsqlview.h"
#include "qsqlfram.h"
#include "property.h"

extern "C"
{
	extern LPTSTR VdbaGetTemporaryFileName();
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define  IDC_SQLCTRL 100

IMPLEMENT_DYNCREATE(CvSqlQuery, CView)

BEGIN_MESSAGE_MAP(CvSqlQuery, CView)
	//{{AFX_MSG_MAP(CvSqlQuery)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
#if defined (_PRINT_ENABLE_)
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
#endif
END_MESSAGE_MAP()

static const int PropertyChange  = 1;
BEGIN_EVENTSINK_MAP(CvSqlQuery, CView)
	ON_EVENT(CvSqlQuery, IDC_SQLCTRL, PropertyChange, OnPropertyChange, VTS_NONE)
END_EVENTSINK_MAP()


CvSqlQuery::CvSqlQuery()
{
}

CvSqlQuery::~CvSqlQuery()
{
}

BOOL CvSqlQuery::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQuery drawing

void CvSqlQuery::OnDraw(CDC* pDC)
{
	CdSqlQuery* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQuery printing

BOOL CvSqlQuery::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvSqlQuery::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CvSqlQuery::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CvSqlQuery diagnostics

#ifdef _DEBUG
void CvSqlQuery::AssertValid() const
{
	CView::AssertValid();
}

void CvSqlQuery::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdSqlQuery* CvSqlQuery::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdSqlQuery)));
	return (CdSqlQuery*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSqlQuery message handlers

int CvSqlQuery::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetDocument();
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	pFrame->SetSqlDoc(pDoc);
	
	if (!theApp.m_pUnknownSqlQuery)
	{
		CuSqlQueryControl ccl;
		HRESULT hError = CoCreateInstance(
			ccl.GetClsid(),
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&(theApp.m_pUnknownSqlQuery));
		if (!SUCCEEDED(hError))
			theApp.m_pUnknownSqlQuery = NULL;
	}

	if (!IsWindow(m_SqlQuery.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_SqlQuery.Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			this,
			IDC_SQLCTRL);
		m_SqlQuery.ShowWindow(SW_SHOW);
		if (!theApp.m_bsqlStreamFile)
		{
			LoadSqlQueryPreferences(m_SqlQuery.GetControlUnknown());
		}

		//
		// Load the global preferences:
		if (theApp.m_bsqlStreamFile)
		{
			IPersistStreamInit* pPersistStreammInit = NULL;
			IUnknown* pUnk = m_SqlQuery.GetControlUnknown();
			HRESULT hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
			if(SUCCEEDED(hr) && pPersistStreammInit)
			{
				theApp.m_sqlStreamFile.SeekToBegin();
				IStream* pStream = theApp.m_sqlStreamFile.GetStream();
				hr = pPersistStreammInit->Load(pStream);
				pPersistStreammInit->Release();
			}
		}

		m_SqlQuery.SetSessionStart(1000);
		m_SqlQuery.SetErrorFileName(VdbaGetTemporaryFileName());
		m_SqlQuery.SetSessionDescription(_T("Ingres Visual DBA invokes Visual SQL control (sqlquery.ocx)"));
		m_SqlQuery.SetHelpFile(VDBA_GetHelpFileName());

		if (pDoc->IsLoadedDoc())
		{
			COleStreamFile& file = pDoc->GetSqlStreamFile();
			IStream* pStream = file.Detach();
			if (pStream)
			{
				m_SqlQuery.Loading((LPUNKNOWN)pStream);
				pStream->Release();
			}
		}
		m_SqlQuery.SetConnectParamInfo(pDoc->GetIngresVersion());
	}
	return 0;
}

void CvSqlQuery::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (IsWindow(m_SqlQuery.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_SqlQuery.MoveWindow(r);
	}
}

void CvSqlQuery::OnPropertyChange() 
{
	theApp.PropertyChange(TRUE);
}
