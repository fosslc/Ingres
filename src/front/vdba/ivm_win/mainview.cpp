/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Ingres Visual Manager
**
**  Source   : mainview.cpp
**  Author   : Sotheavut UK (uk$so01
**
**  implementation file ( View that holds a main dialog)
**
** History:
** 14-Dec-1998 (uk$so01)
**    (created)
** 22-Dec-2000 (noifr01)
**    (SIR 103548) appended the installation ID in the application title.
** 05-Apr-2001 (uk$so01)
**    Move the code of getting installation ID to ivmdml (.h, .cpp) to be more 
**    generic and reusable.
**
*/

#include "stdafx.h"
#include "ivm.h"
#include "maindoc.h"
#include "mainfrm.h"
#include "mainview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvMainView, CView)

BEGIN_MESSAGE_MAP(CvMainView, CView)
	//{{AFX_MSG_MAP(CvMainView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvMainView construction/destruction

CvMainView::CvMainView()
{
	m_pDlgMain = NULL;
	m_nPage = 1;
}

CvMainView::~CvMainView()
{
}

BOOL CvMainView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView drawing

void CvMainView::OnDraw(CDC* pDC)
{
	CdMainDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CString strTitle;
	strTitle.LoadString (AFX_IDS_APP_TITLE);
	strTitle+= theApp.GetCurrentInstallationID();
	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView printing

BOOL CvMainView::OnPreparePrinting(CPrintInfo* pInfo)
{
//	pInfo->SetMaxPage(2); 
	BOOL bRet = DoPreparePrinting(pInfo);
	pInfo->m_nNumPreviewPages = 2;
	return bRet;
}

void CvMainView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	m_nPage = 1;
	CTime t = CTime::GetCurrentTime();
	m_strPrintTime = t.Format (_T("%a %b %d %H:%M:%S %Y"));
	// TODO: add extra initialization before printing
}

void CvMainView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	m_nPage = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CvMainView diagnostics

#ifdef _DEBUG
void CvMainView::AssertValid() const
{
	CView::AssertValid();
}

void CvMainView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdMainDoc* CvMainView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdMainDoc)));
	return (CdMainDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvMainView message handlers

int CvMainView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		CdMainDoc* pDoc = GetDocument();
		if (pDoc)
		{
			pDoc->m_hwndMainFrame = GetParentFrame()->m_hWnd;
			pDoc->LoadEvents();
			theApp.m_setting.Load();
		}
		m_pDlgMain = new CuDlgMain(this);
		m_pDlgMain->Create (IDD_MAIN, this);
		m_pDlgMain->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvMainView::OnCreate): Cannot create view"));
		return -1;
	}
	return 0;
}

void CvMainView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pDlgMain && IsWindow (m_pDlgMain->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pDlgMain->MoveWindow (r);
	}
}


void CvMainView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	if (pInfo->m_nCurPage == 1)  // page no. 1 is the title page
	{
		PrintTitlePage(pDC, pInfo);
		return; // nothing else to print on page 1 but the page title
	}

	CString strHeader = _T("IVM Report");
	PrintPageHeader(pDC, pInfo, strHeader);

	/*
	CString strHeader = GetDocument()->GetTitle();

	PrintPageHeader(pDC, pInfo, strHeader);
	// PrintPageHeader() subtracts out from the pInfo->m_rectDraw the
	// amount of the page used for the header.

	pDC->SetWindowOrg(pInfo->m_rectDraw.left,-pInfo->m_rectDraw.top);

	// Now print the rest of the page
	OnDraw(pDC);	
	*/
}

void CvMainView::PrintTitlePage(CDC* pDC, CPrintInfo* pInfo)
{
	// Prepare a font size for displaying the file name
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(LOGFONT));
//	logFont.lfHeight = 75;  //  3/4th inch high in MM_LOENGLISH
							// (1/100th inch)
	logFont.lfHeight = 200; // 2cm
	CFont font;
	CFont* pOldFont = NULL;
	if (font.CreateFontIndirect(&logFont))
		pOldFont = pDC->SelectObject(&font);

	// Get the file name, to be displayed on title page
//	CString strPageTitle = GetDocument()->GetTitle();
	CString strPageTitle = _T("Ingres Visual Manager");

	// Display the file name 1 inch below top of the page,
	// centered horizontally
	pDC->SetTextAlign(TA_CENTER);
	pDC->TextOut(pInfo->m_rectDraw.right/2, -100, strPageTitle);

	if (pOldFont != NULL)
		pDC->SelectObject(pOldFont);
}

void CvMainView::PrintPageHeader(CDC* pDC, CPrintInfo* pInfo, CString& strHeader)
{
	TEXTMETRIC textMetric;
	pDC->GetTextMetrics(&textMetric);
	// Print a page header consisting of the name of
	// the document and a horizontal line
	pDC->SetTextAlign(TA_LEFT);
//	pDC->TextOut(0,-25, strHeader);  // 1/4 inch down
	pDC->TextOut(100,-100, strHeader);  // 1/4 inch down

	CRect r (100, -100, pInfo->m_rectDraw.right, -100 - textMetric.tmHeight);
	pDC->DrawText(m_strPrintTime, r, DT_RIGHT);


	// Draw a line across the page, below the header
//	int y = -35 - textMetric.tmHeight;          // line 1/10th inch below text
	int y = -110 - textMetric.tmHeight;          // line 1/10th inch below text
	pDC->MoveTo(100, y);                          // from left margin
	pDC->LineTo(pInfo->m_rectDraw.right, y);    // to right margin

	// Subtract out from the drawing rectange the space used by the header.
//	y -= 25;    // space 1/4 inch below (top of) line
	y -= 1000;    //10 cm below (top of) line
	pInfo->m_rectDraw.top += y;
	CString strText;
	strText.Format (_T("Print document not available yet: %d"), pInfo->m_nCurPage);
	pDC->TextOut(100, y, strText);  // 1/4 inch down
	
	PrintPageNumber(pDC, pInfo);
}

void CvMainView::PrintPageNumber(CDC* pDC, CPrintInfo* pInfo)
{
	TEXTMETRIC textMetric;
	pDC->GetTextMetrics(&textMetric);
	int y = pInfo->m_rectDraw.bottom + 100;
	CRect r(0, y, pInfo->m_rectDraw.right, y - textMetric.tmHeight);
	CString strPage;
	strPage.Format (_T("Page %d/2"), pInfo->m_nCurPage);
	pDC->DrawText(strPage, r, DT_CENTER);
}



void CvMainView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnPrepareDC(pDC, pInfo);
	pDC->SetMapMode (MM_LOMETRIC);
	if (pInfo)
	{
		if (pInfo->m_nCurPage > m_nPage)
			m_nPage++;

		if (m_nPage <= 2)
			pInfo->m_bContinuePrinting = TRUE;
		else
			pInfo->m_bContinuePrinting = FALSE;
	}
}

