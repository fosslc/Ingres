/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmview.cpp : implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : implementation of the view CvIpm class  (MFC frame/view/doc).
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
*/

#include "stdafx.h"
#include "xipm.h"
#include "ipmdoc.h"
#include "ipmview.h"
#include "mainfrm.h"
#include "ingobdml.h"
#include "compfile.h"
#include ".\ipmview.h"

#define  IDC_IPMCTRL 100
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvIpm, CView)

BEGIN_MESSAGE_MAP(CvIpm, CView)
	//{{AFX_MSG_MAP(CvIpm)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
#if defined (_PRINT_ENABLE_)
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
#endif
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

const int TreeSelChange = 1;
const int PropertiesChange = 2;

BEGIN_EVENTSINK_MAP(CvIpm, CView)
	ON_EVENT(CvIpm, IDC_IPMCTRL, TreeSelChange,     OnIpmTreeSelChange, VTS_NONE)
	ON_EVENT(CvIpm, IDC_IPMCTRL, PropertiesChange,  OnPropertiesChange, VTS_NONE)
END_EVENTSINK_MAP()


/////////////////////////////////////////////////////////////////////////////
// CvIpm construction/destruction

CvIpm::CvIpm()
{
}

CvIpm::~CvIpm()
{
}

BOOL CvIpm::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvIpm drawing

void CvIpm::OnDraw(CDC* pDC)
{
	CdIpm* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString strWorkingFile = pDoc->GetWorkingFile();

	CString strTitle; 
	strTitle.LoadString (AFX_IDS_APP_TITLE);
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
// CvIpm printing

BOOL CvIpm::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvIpm::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CvIpm::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CvIpm diagnostics

#ifdef _DEBUG
void CvIpm::AssertValid() const
{
	CView::AssertValid();
}

void CvIpm::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdIpm* CvIpm::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdIpm)));
	return (CdIpm*)m_pDocument;
}
#endif //_DEBUG


void CvIpm::CreateIpmCtrl()
{
	CRect r;
	GetClientRect (r);
	m_cIpm.Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this,
		IDC_IPMCTRL);

	CdIpm* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CString strWorkingFile = pDoc->GetWorkingFile();
	if (strWorkingFile.IsEmpty())
	{
		//
		// Load the propertis from file IIGUIS.PRF:
		OpenStreamInit (m_cIpm.GetControlUnknown(), _T("Ipm"));
	}
	CString csLocalSqlErrorFileName;
	csLocalSqlErrorFileName = pDoc->GetClassFilesErrors()->GetLocalErrorFile();
	m_cIpm.SetErrorlogFile(csLocalSqlErrorFileName);
	m_cIpm.SetSessionDescription(_T("Ingres Visual Performance Monitor (ipm.ocx)"));
	m_cIpm.SetSessionStart(1000);
	m_cIpm.SetHelpFile("vdbamon.chm");
}


/////////////////////////////////////////////////////////////////////////////
// CvIpm message handlers

int CvIpm::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	pFrame->SetIpmDoc(GetDocument());

#if defined (_INITIATE_CONTROL_ON_CREATE)
	CreateIpmCtrl();
	m_cIpm.ShowWindow(SW_HIDE);
#endif
	return 0;
}

void CvIpm::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (IsWindow(m_cIpm.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_cIpm.MoveWindow(r);
	}
}

void CvIpm::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
	short arrayFilter[5];
	CdIpm* pDoc = GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	if (nHint == 4)
	{
		Invalidate();
		return;
	}

	CString strMsg;
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

	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	pFrame->GetFilters(arrayFilter, 5);

	if (nHint == 1 && IsWindow (m_cIpm.m_hWnd))
	{
		if (pDoc->IsConnected())
			return;
		m_cIpm.UpdateFilters(arrayFilter, 5);
		BOOL bOK = m_cIpm.Initiate(strNode, strServer, strUser, NULL);
		pDoc->SetConnectState(bOK);
		if (!bOK)
		{
			m_cIpm.ShowWindow(SW_HIDE);
			strMsg.Format (IDS_CANNOT_CONNECT_TO, (LPCTSTR)strNode);
			SetForegroundWindow();
			AfxMessageBox (strMsg);
		}
		Invalidate();
	}
	else
	if (nHint == 2) // Called from the command line arg
	{
		if( IsWindow (m_cIpm.m_hWnd))
		{
			m_cIpm.ShowWindow(SW_HIDE);
		}
		else
		{
			CreateIpmCtrl();
		}
		m_cIpm.ShowWindow(SW_SHOW);
		m_cIpm.UpdateFilters(arrayFilter, 5);
		BOOL bOK = m_cIpm.Initiate(strNode, strServer, strUser, NULL);
		pDoc->SetConnectState(bOK);
		if (!bOK)
		{
			m_cIpm.ShowWindow(SW_HIDE);
			strMsg.Format (IDS_CANNOT_CONNECT_TO, (LPCTSTR)strNode);
			SetForegroundWindow();
			AfxMessageBox (strMsg);
		}
		Invalidate();
	}
	else
	if (nHint == 3) // For loading
	{
		if(!IsWindow (m_cIpm.m_hWnd))
		{
			CreateIpmCtrl();
		}
		m_cIpm.ShowWindow(SW_SHOW);
		m_cIpm.UpdateFilters(arrayFilter, 5);
		Invalidate();
	}
	else
	if (nHint == 5) // Resizing
	{
		if (IsWindow(m_cIpm.m_hWnd))
		{
			CRect r;
			GetClientRect (r);
			m_cIpm.MoveWindow(r);
		}
	}
}


void CvIpm::OnIpmTreeSelChange() 
{
	TRACE0("CvIpm::OnIpmTreeSelChange\n");
	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	ASSERT(pFrame);
	if (!pFrame)
		return;
	pFrame->DisableExpresRefresh();
}

void CvIpm::OnPropertiesChange()
{
	CfMainFrame* pFrame = (CfMainFrame*)GetParentFrame();
	ASSERT(pFrame);
	if (!pFrame)
		return;
	pFrame->PropertiesChange();
}

void CvIpm::OnSysColorChange()
{
	CView::OnSysColorChange();
	if (IsWindow(m_cIpm.m_hWnd))
		m_cIpm.SendMessageToDescendants(WM_SYSCOLORCHANGE);
}
