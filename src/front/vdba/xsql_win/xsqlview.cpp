/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xsqlview.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : implementation of the CvSqlQuery class
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 02-Aug-2004 (uk$so01)
**    BUG #112765, ISSUE 13364531 (Activate the print of sql test)
** 01-Sept-2004 (schph01)
**    SIR #106648 set the error file name for managing the 'History of SQL
**    Statements Errors' dialog.
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
*/


#include "stdafx.h"
#include "xsql.h"
#include "mainfrm.h"
#include "xsqldoc.h"
#include "xsqlview.h"
#include "ingobdml.h"
#include "compfile.h"
#include ".\xsqlview.h"

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
	ON_COMMAND(ID_FILE_PRINT, DoPrint/*CView::OnFilePrint*/)
	ON_COMMAND(ID_FILE_PRINT_DIRECT,  DoPrint/*CView::OnFilePrint*/)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
#endif
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_WM_SYSCOLORCHANGE()
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
	CString strWorkingFile = pDoc->GetWorkingFile();
	CString strTitle; 
	strTitle.LoadString (IDS_APP_TITLE_XX);
	strTitle += INGRESII_QueryInstallationID();
	if (!strWorkingFile.IsEmpty())
	{
		strTitle += _T(" - ");
		strTitle += strWorkingFile;
	}
	else
	{
		CString strDefault;
		CString strNode = pDoc->GetNode();
		CString strServer = pDoc->GetServer();
		CString strUser = pDoc->GetUser();

		strDefault.LoadString(IDS_DEFAULT_SERVER);
		if (strDefault.CompareNoCase(strServer) == 0)
			strServer = _T("");
		strDefault.LoadString(IDS_DEFAULT_USER);
		if (strDefault.CompareNoCase(strUser) == 0)
			strUser = pDoc->GetDefaultConnectedUser();

		CString strItem;
		CString strConnection = _T("");
		if (!strNode.IsEmpty())
		{
			strConnection += strNode;
		}
		if (!strServer.IsEmpty())
		{
			strItem.Format(_T(" /%s"), (LPCTSTR)strServer);
			strConnection += strItem;
		}
		if (!strConnection.IsEmpty())
		{
			strItem.Format(_T(" - [%s]"), (LPCTSTR)strConnection);
			strTitle += strItem;
		}
	}
	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);
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
	USES_CONVERSION;
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	pFrame->SetSqlDoc(GetDocument());
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
		m_SqlQuery.SetSessionStart(1000);
		m_SqlQuery.SetSessionDescription(_T("Ingres Visual SQL (sqlquery.ocx)"));
		m_SqlQuery.SetHelpFile("vdbasql.chm");

		CdSqlQuery* pDoc = GetDocument();
		ASSERT_VALID(pDoc);

		CString csLocalSqlErrorFileName;
		csLocalSqlErrorFileName = pDoc->GetClassFilesErrors()->GetLocalErrorFile();
		m_SqlQuery.SetErrorFileName((LPTSTR)(LPCTSTR)csLocalSqlErrorFileName);

		CaSessionManager& sessionMgr = theApp.GetSessionManager();
		sessionMgr.SetDescription(_T("Ingres Visual SQL"));

		//
		// Load the propertis from file IIGUIS.PRF:
		OpenStreamInit (m_SqlQuery.GetControlUnknown(), _T("SqlQuery"));
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
	TRACE0("CvSqlQuery::OnPropertyChange\n");
	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	ASSERT(pFrame);
	if (!pFrame)
		return;
	pFrame->PropertyChange();
}

void CvSqlQuery::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
	if (nHint == 1)
	{
		Invalidate();
		return;
	}
}

void CvSqlQuery::DoPrint()
{
	m_SqlQuery.Print();
}


void CvSqlQuery::OnUpdateFilePrint(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if (IsWindow(m_SqlQuery.m_hWnd))
		bEnable = m_SqlQuery.IsPrintEnable();

	pCmdUI->Enable(bEnable);
}

void CvSqlQuery::OnSysColorChange()
{
	CView::OnSysColorChange();
	if (IsWindow(m_SqlQuery.m_hWnd))
		m_SqlQuery.SendMessageToDescendants(WM_SYSCOLORCHANGE);
}
