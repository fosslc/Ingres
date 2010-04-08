/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qlowview.cpp, Implementation file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Lower view of the sql frame. It contains a CTabCtrl to display the 
**               history of result of the queries
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 25-Aug-2004 (uk$so01)
**    BUG #112891,  The print of vdba/sqltest and vdbasql don't print correctly
**    the page numbers.
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qlowview.h"
#include "qredoc.h"
#include "qpageres.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvSqlQueryLowerView, CView)
CvSqlQueryLowerView::CvSqlQueryLowerView()
{
	m_pQueryResultDlg = NULL;
}

CvSqlQueryLowerView::~CvSqlQueryLowerView()
{
}


BEGIN_MESSAGE_MAP(CvSqlQueryLowerView, CView)
	//{{AFX_MSG_MAP(CvSqlQueryLowerView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryLowerView diagnostics

#ifdef _DEBUG
void CvSqlQueryLowerView::AssertValid() const
{
	CView::AssertValid();
}

void CvSqlQueryLowerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryLowerView message handlers

int CvSqlQueryLowerView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	try
	{
		m_pQueryResultDlg = new CuDlgSqlQueryResult (this);
		m_pQueryResultDlg->Create (IDD_SQLQUERY_RESULT, this);
		m_pQueryResultDlg->ShowWindow (SW_SHOW);
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQL_RESULT, MB_ICONEXCLAMATION|MB_OK);
		m_pQueryResultDlg = NULL;
	}
	return 0;
}

void CvSqlQueryLowerView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (!m_pQueryResultDlg)
		return;
	if (!IsWindow (m_pQueryResultDlg->m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_pQueryResultDlg->MoveWindow (r);
}

BOOL CvSqlQueryLowerView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

void CvSqlQueryLowerView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	switch (nHint)
	{
	case UPDATE_HINT_LOAD:
		if (pHint && pHint == pDoc)
		{
			//
			// Load document from the serialization:
			m_pQueryResultDlg->LoadDocument();
		}
		break;
/*UKS
	case UPDATE_HINT_SETTINGCHANGE:
		if (pHint && pHint == pDoc)
			m_pQueryResultDlg->SettingChange(pDoc);
		break;
*/

	default:
		break;
	}
}

void CvSqlQueryLowerView::PrintPage(CDC* pDC, int pageNum)
{
	ASSERT (pDC->IsPrinting());
	if (!pDC->IsPrinting()) {
		return;
	}

	// Here, printing takes place
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	ASSERT (pDoc);

	// Select the printer font
	CFont* pOldFont = pDC->SelectObject(&m_printerFont);

	// Recalculate page useable height
	int pageDataHeight = m_nPageHeight - m_nHeaderHeight - m_nFooterHeight;

	CPoint pOrig = pDC->GetViewportOrg();
	// Set dc viewport origin
	int xOrg = 0;
	int yOrg = pageDataHeight * (pageNum - 1);    // nth page
	yOrg -= m_nHeaderHeight;                      // minus header height
	pDC->SetViewportOrg(-xOrg, -yOrg);

	// Select clipping region, taking scaling factor in account
	int x1 = 0;
	int x2 = x1 + m_nPageWidth;
	int y1 = m_nHeaderHeight;
	int y2 = y1 + pageDataHeight;
	x1 = x1 >> m_ScalingFactorX;
	x2 = x2 >> m_ScalingFactorX;
	y1 = y1 >> m_ScalingFactorY;
	y2 = y2 >> m_ScalingFactorY;
	CRgn region;
	BOOL bSuccess = region.CreateRectRgn(x1, y1, x2, y2);
	ASSERT (bSuccess);
	int res = pDC->SelectClipRgn(&region);
	ASSERT (res != ERROR);
	ASSERT (res != NULLREGION);

	int x=0, y=0;

	QueryPageDataType pgType = QUERYPAGETYPE_NONE;
	if (m_pPageData)
		pgType = m_pPageData->GetQueryPageType();

	// Head lines
	if (pgType == QUERYPAGETYPE_SELECT ||  pgType == QUERYPAGETYPE_NONSELECT || pgType == QUERYPAGETYPE_QEP) {
		// Database caption
		pDC->SelectObject(&m_printerCaptFont);
		CString csDatabaseCapt(_T("Connected Database:"));
		pDC->TextOut(x, y, csDatabaseCapt);
		y += m_nCaptLineHeight;
		y += m_nCaptLineHeight/2;
		pDC->SelectObject(&m_printerFont);

		// Database
		if (m_pPageData)
			pDC->TextOut(x, y, m_pPageData->m_strDatabase);
		else
			pDC->TextOut(x, y, pDoc->m_strDatabase);
		y += m_nLineHeight;
		y += m_nLineHeight; // Extra blank line

		// Statement caption
		pDC->SelectObject(&m_printerCaptFont);
		CString csStatementCapt(_T("SQL Query Text:"));
		pDC->TextOut(x, y, csStatementCapt);
		y += m_nCaptLineHeight;
		y += m_nCaptLineHeight/2;
		pDC->SelectObject(&m_printerFont);

		// Statement - only current statement printed
		CString csFullStatement, csCurrentStatement;
		if (m_pPageData) {
			csFullStatement = m_pPageData->m_strStatement;
			csCurrentStatement = csFullStatement;;
		}
		else {
			csFullStatement = pDoc->m_strStatement;
			csCurrentStatement = csFullStatement.Mid(pDoc->m_nStart, pDoc->m_nEnd-pDoc->m_nStart);
		}
		// pDC->TextOut(x, y, csFullStatement);
		// y += m_nLineHeight;

		MultilineTextOut(pDC, x, y, csCurrentStatement, pageDataHeight); // updates y

		y += m_nLineHeight; // Extra blank line
	}

	// following code is according to result type
	switch (pgType)
	{
	case QUERYPAGETYPE_SELECT:
		PrintSelect(pDC, pageNum, x, y, (CaQuerySelectPageData*)m_pPageData, pageDataHeight);
		break;
	case QUERYPAGETYPE_NONSELECT:
		PrintNonSelect(pDC, pageNum, x, y, (CaQueryNonSelectPageData*)m_pPageData, pageDataHeight);
		break;
	case QUERYPAGETYPE_QEP:
		PrintQep(pDC, pageNum, x, y, (CaQueryQepPageData*)m_pPageData, pageDataHeight);
		break;
	case QUERYPAGETYPE_RAW:
		PrintRaw(pDC, pageNum, x, y, (CaQueryRawPageData*)m_pPageData, pageDataHeight);
		break;
	case QUERYPAGETYPE_NONE:
		//PrintNone(pDC, pageNum, x, y);
		break;

	default:
		ASSERT (FALSE);
	}

	// deselect clipping region and delete it
	res = pDC->SelectClipRgn(NULL, RGN_COPY);
	ASSERT (res != ERROR);
	region.DeleteObject();

	// Deselect the printer font
	pDC->SelectObject(pOldFont);

	// Update number of pages to be printed, rounded up
	m_nbPrintPage = (y + (pageDataHeight-1) ) / pageDataHeight;

	pDC->SetViewportOrg(pOrig.x, pOrig.y);
}

void CvSqlQueryLowerView::OnDraw(CDC* pDC)
{

}

void CvSqlQueryLowerView::DoFilePrintPreview()
{
	OnFilePrintPreview();
}



void CvSqlQueryLowerView::DoFilePrint()
{
	CDC dc;
	CPrintDialog printDlg(FALSE);

	if (printDlg.DoModal() == IDCANCEL)         // Get printer settings from user
		return;

	dc.Attach(printDlg.GetPrinterDC());         // Attach a printer DC
	dc.m_bPrinting = TRUE;

	CString strTitle;                           // Get the application title
	strTitle.LoadString(AFX_IDS_APP_TITLE);

	DOCINFO di;                                 // Initialise print document details
	::ZeroMemory (&di, sizeof (DOCINFO));
	di.cbSize = sizeof (DOCINFO);
	di.lpszDocName = strTitle;

	BOOL bPrintingOK = dc.StartDoc(&di);        // Begin a new print job
	// Get the printing extents and store in the m_rectDraw field of a  CPrintInfo object
	CPrintInfo Info;
	Info.m_rectDraw.SetRect(0,0, dc.GetDeviceCaps(HORZRES), dc.GetDeviceCaps(VERTRES));
	Info.m_bPreview = TRUE;

	OnBeginPrinting(&dc, &Info);                // Call your "Init printing" funtion
	if (m_nPageWidth > Info.m_rectDraw.Width())
		m_nPageWidth = Info.m_rectDraw.Width();
	if (m_nPageHeight > Info.m_rectDraw.Height())
		m_nPageHeight = Info.m_rectDraw.Height();

	while (bPrintingOK)
	{
		dc.StartPage();                         // begin new page
		PrintPageHeader(&dc, Info.m_nCurPage);  // updates m_nHeaderHeight
		PrintPageFooter(&dc, Info.m_nCurPage);  // updates m_nFooterHeight

		ASSERT (m_nHeaderHeight >= 0);
		ASSERT (m_nFooterHeight >= 0);

		PrintPage(&dc, Info.m_nCurPage);
		Info.m_nCurPage++;
		dc.EndPage();
		bPrintingOK = ((int)Info.m_nCurPage > m_nbPrintPage) ? FALSE: TRUE;
	}

	OnEndPrinting(&dc, &Info);
	dc.EndDoc();
	dc.Detach();
}

BOOL CvSqlQueryLowerView::OnPreparePrinting(CPrintInfo* pInfo) 
{
	return DoPreparePrinting(pInfo);
}

void CvSqlQueryLowerView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	// Necessary to pick informations from the printer dc
	pDC->SetMapMode(MM_TEXT);
	// Pick printer information
	m_ScalingFactorX = pDC->GetDeviceCaps(SCALINGFACTORX);  // power of 2
	m_ScalingFactorY = pDC->GetDeviceCaps(SCALINGFACTORY);  // power of 2
	m_nPageWidth = pDC->GetDeviceCaps(PHYSICALWIDTH) - 2*pDC->GetDeviceCaps(PHYSICALOFFSETX);
	m_nPageHeight = pDC->GetDeviceCaps(PHYSICALHEIGHT) - 2*pDC->GetDeviceCaps(PHYSICALOFFSETY);
	//
	// create printer font and store useful information
	//

	// Get screen font
	HFONT hScreenFont = NULL;
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	ASSERT (pDoc);
	LOGFONT lf;
	hScreenFont = pDoc->GetProperty().GetFont();
	if (!hScreenFont)
	{
		CWnd* pCurrentPage = m_pQueryResultDlg->GetCurrentPage();
		if (pCurrentPage)
			hScreenFont = (HFONT)pCurrentPage->SendMessage (WMUSRMSG_GETFONT, 0, 0); 
		if (!hScreenFont)
			hScreenFont = (HFONT)GetFont();
	}
	ASSERT (hScreenFont);
	// Create wysiwig printer fonts
	CFont* pScreenFont = CFont::FromHandle(hScreenFont);
	int success = pScreenFont->GetLogFont(&lf);
	ASSERT (success);

	CDC* pScreenDC = GetDC();
	ASSERT (pScreenDC);
	int printerLogPixelsX = pDC->GetDeviceCaps(LOGPIXELSX);
	int screenLogPixelsX  = pScreenDC->GetDeviceCaps(LOGPIXELSX);
	int printerLogPixelsY = pDC->GetDeviceCaps(LOGPIXELSY);
	int screenLogPixelsY  = pScreenDC->GetDeviceCaps(LOGPIXELSY);
	ReleaseDC(pScreenDC);
	
	// Scales: rounded to the nearest for print scales, rounded up for font
	m_PixelXPrintScale = ( ( printerLogPixelsX + screenLogPixelsX / 2 - 1) / screenLogPixelsX) << m_ScalingFactorX;
	m_PixelYPrintScale = ( ( printerLogPixelsY + screenLogPixelsY / 2 - 1) / screenLogPixelsY) << m_ScalingFactorY;
	int fontYScale = ( (printerLogPixelsY + screenLogPixelsY - 1) / screenLogPixelsY ) << m_ScalingFactorY;
	lf.lfHeight *= fontYScale;
	
	// Normal font
	lf.lfWeight = FW_NORMAL;      // So that displayed result is not too heavy looking at - SIDE EFFECT IF SCREEN FONT IS BOLD
	lf.lfUnderline = FALSE;
	BOOL bSuccess = m_printerFont.CreateFontIndirect(&lf);
	ASSERT (bSuccess);
	
	// underlined font
	lf.lfUnderline = TRUE;
	bSuccess = m_printerCaptFont.CreateFontIndirect(&lf);
	ASSERT (bSuccess);
	
	// Bold font
	lf.lfUnderline = FALSE;
	lf.lfWeight = FW_BOLD;
	bSuccess = m_printerBoldFont.CreateFontIndirect(&lf);
	ASSERT (bSuccess);
	
	// calculate reference text sizes
	TEXTMETRIC tm;
	CFont* pOldFont = pDC->SelectObject(&m_printerFont);
	pDC->GetTextMetrics(&tm);
	m_nLineHeight = tm.tmHeight + tm.tmExternalLeading;
	pDC->SelectObject(&m_printerCaptFont);
	pDC->GetTextMetrics(&tm);
	m_nCaptLineHeight = tm.tmHeight + tm.tmExternalLeading;
	pDC->SelectObject(&m_printerBoldFont);
	pDC->GetTextMetrics(&tm);
	m_nBoldLineHeight = tm.tmHeight + tm.tmExternalLeading;
	pDC->SelectObject(pOldFont);
	
	// Reset number of pages to be printed, will be calculated at first call to PrintPage()
	m_nbPrintPage = 0;
	
	// Call special method of CuDlgSqlQueryResult, in order to get current property page data
	// Code here rather than in OnPrint() so it will not be called for each printed page
	m_pPageData = m_pQueryResultDlg->GetCurrentPageData();
	// Can be null (freshly created sql with no statement executed yet
}

void CvSqlQueryLowerView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	if (m_pPageData)
		delete m_pPageData;
	// Free gdi resources
	m_printerFont.DeleteObject();
	m_printerCaptFont.DeleteObject();
	m_printerBoldFont.DeleteObject();
}


void CvSqlQueryLowerView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo) 
{
	// Derived for print-time pagination, plus viewport management
	CView::OnPrepareDC(pDC, pInfo);
	if (pDC->IsPrinting()) {

		pDC->SetMapMode(MM_TEXT);
		// Manage printing continue/stop
		if (pInfo->m_nCurPage > 1) {
			ASSERT (m_nbPrintPage > 0);
		if ((int)pInfo->m_nCurPage > m_nbPrintPage)
			pInfo->m_bContinuePrinting = FALSE;
		else
			pInfo->m_bContinuePrinting = TRUE;
		}

		else
			pInfo->m_bContinuePrinting = TRUE;  // Always print first page
	}
}

void CvSqlQueryLowerView::PrintPageHeader(CDC* pDC, int pageNum)
{
	// Select the printer font
	CFont* pOldFont = pDC->SelectObject(&m_printerFont);

	// Title on the left
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	ASSERT (pDoc);
	CString csHeader = pDoc->GetTitle();
	pDC->TextOut(0, 0, csHeader);

	// Page number right-aligned
	CString csPageNum;
	csPageNum.Format(_T("Page No %d"), pageNum);
	CSize size = pDC->GetTextExtent(csPageNum);
	int hPos = m_nPageWidth - size.cx;
	pDC->TextOut(hPos, 0, csPageNum);

	// underline full width of page
	pDC->MoveTo(0, m_nLineHeight);
	pDC->LineTo(m_nPageWidth, m_nLineHeight);

	// Deselect the printer font
	pDC->SelectObject(pOldFont);

	// MANDATORY : MUST update m_nHeaderHeight
	m_nHeaderHeight = m_nLineHeight * 3;
}

void CvSqlQueryLowerView::PrintPageFooter(CDC* pDC, int pageNum)
{
	// Select the printer font
	CFont* pOldFont = pDC->SelectObject(&m_printerFont);

	// EMPTY - if code in there, MUST update m_nFooterHeight

	// Deselect the printer font
	pDC->SelectObject(pOldFont);

	// MANDATORY : MUST update m_nFooterHeight
	m_nFooterHeight = 0;
}

void CvSqlQueryLowerView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	// Pre-adjust width and height
	if (m_nPageWidth > pInfo->m_rectDraw.Width())
		m_nPageWidth = pInfo->m_rectDraw.Width();
	if (m_nPageHeight > pInfo->m_rectDraw.Height())
		m_nPageHeight = pInfo->m_rectDraw.Height();

	PrintPageHeader(pDC, pInfo->m_nCurPage);  // updates m_nHeaderHeight
	PrintPageFooter(pDC, pInfo->m_nCurPage);  // updates m_nFooterHeight

	ASSERT (m_nHeaderHeight >= 0);
	ASSERT (m_nFooterHeight >= 0);

	PrintPage(pDC, pInfo->m_nCurPage);
}

// Specific print methods according to pane type
void CvSqlQueryLowerView::PrintSelect(CDC* pDC, int pageNum, int x, int &y, CaQuerySelectPageData* pPageData, int pageDataHeight)
{
	ASSERT (pPageData->IsKindOf(RUNTIME_CLASS(CaQuerySelectPageData)));

	// Tuples caption
	pDC->SelectObject(&m_printerCaptFont);
	CString csTuplesCapt(_T("Resulting Tuples:"));
	pDC->TextOut(x, y, csTuplesCapt);
	y += m_nCaptLineHeight;
	y += m_nCaptLineHeight/2;
	pDC->SelectObject(&m_printerFont);
	y += m_nLineHeight; // Extra blank line

	//
	// Tuples
	//

	// Tuples header:
	// Note: pPageData->m_listRows.m_nHeaderCount CANNOT BE USED - must loop on position

	// Calculate columns margins for a nice frame rectangle display
	int colMarginX = (2+2) * m_PixelXPrintScale; // Spare 2 pixels left and 2 pixels right between text and frame
	int colMarginY = (1+1) * m_PixelYPrintScale; // Spare 1 pixel top and 1 pixel bottom between text and frame
	ASSERT (colMarginX % 2 == 0);   // must be even
	ASSERT (colMarginY % 2 == 0);   // must be even

	/* ////////////// TO BE KEPT - EMPHASIZES PRINTER PROBLEM IN ALIGNMENT

	// Draw rectangle
	CRect rect(2010,
				 y,
				 2208,
				 y + m_nLineHeight + colMarginY
				 );
	rect.right++; rect.bottom++;
	pDC->Rectangle(rect);
	rect.right--; rect.bottom--;

	// Draw text
	rect.DeflateRect(colMarginX / 2, colMarginY / 2);
	UINT nFormat = DT_VCENTER | DT_SINGLELINE | DT_LEFT;
	CString cs = "Erreur!";
	pDC->DrawText(cs, rect, nFormat);
	y += m_nLineHeight + colMarginY;
	return;


	////////////// TO BE KEPT - EMPHASIZES PRINTER PROBLEM IN ALIGNMENT */


	POSITION posHeader = pPageData->m_listRows.m_listHeaders.GetHeadPosition();
	int colCount = 0;
	int usedWidth = 0;
	while (posHeader) {
		CaQueryResultRowsHeader* pHeader = pPageData->m_listRows.m_listHeaders.GetNext(posHeader);
		ASSERT (pHeader);
		int printColWidth = pHeader->m_nWidth * m_PixelXPrintScale;
		if (printColWidth + colMarginX + usedWidth > m_nPageWidth)
			break;  // Don't use this column, stop the loop

		// Draw rectangle
		CRect rect(usedWidth, y, usedWidth + printColWidth + colMarginX, y + m_nBoldLineHeight + colMarginY);
		rect.right++; rect.bottom++;
		pDC->Rectangle(rect);
		rect.right--; rect.bottom--;

		// Draw text
		pDC->SelectObject(&m_printerBoldFont);
		rect.DeflateRect(colMarginX / 2, colMarginY / 2);
		pDC->DrawText(pHeader->m_strHeader, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		pDC->SelectObject(&m_printerFont);
		// Update loop variables
		usedWidth += printColWidth + colMarginX;
		colCount++;
	}
	// One line has been drawn
	y += m_nBoldLineHeight + colMarginY;

	// Now colCount contains number of displayable columns according to page width
	ASSERT (colCount > 0);

	// Tuples
	POSITION posRow = pPageData->m_listRows.m_listAdjacent.GetHeadPosition();
	BOOL bFirstTime2 = TRUE;
	int curCol = 0;
	for (curCol=0; posRow; curCol++) {
		// Skip uninteresting lines (n/a , etc...)
		CStringList* pRow = pPageData->m_listRows.m_listAdjacent.GetNext(posRow);
		ASSERT (pRow);
		int rowData = pPageData->m_listRows.m_aItemData[curCol];
		BOOL bSkip = FALSE;
		switch (rowData) 
		{
		case 0:   // normal lines: print
			break;
		case 1:   // head lines : skip if n/a
			{
			POSITION posCol = pRow->GetHeadPosition();
			CString cs = pRow->GetNext(posCol);
			if (cs == _T("n/a"))
				bSkip = TRUE;
			}
			break;
		case 2:
			if (bFirstTime2)
				bFirstTime2 = FALSE;
			else
				bSkip = TRUE;
			break;
		default:
			ASSERT (FALSE);
			break;
		}
		if (bSkip)
			continue;   // skip the line (continue the for() )


		// First, check line to be printed does not overlap 2 pages
		int curLinePage = y / pageDataHeight;
		int nextLinePage = (y + m_nLineHeight + colMarginY) / pageDataHeight;
		ASSERT (nextLinePage >= curLinePage);
		if (nextLinePage > curLinePage)
			y = nextLinePage * pageDataHeight;

		POSITION posCol = pRow->GetHeadPosition();
		posHeader = pPageData->m_listRows.m_listHeaders.GetHeadPosition();
		// use the first "colCount" columns
		usedWidth = 0;
		for (int cptCol = 0; cptCol < colCount; cptCol++) {
			// Pick column width and align info
			CaQueryResultRowsHeader* pHeader = pPageData->m_listRows.m_listHeaders.GetNext(posHeader);
			ASSERT (pHeader);
			int printColWidth = pHeader->m_nWidth * m_PixelXPrintScale;
			int printAlign = pHeader->m_nAlign;

			// Pick column displayable text
			CString cs = pRow->GetNext(posCol);
			if (rowData == 2)
				cs.LoadString(IDS_NEXT_TUPLES_UNAVAILABLE); //= _T("< Next tuples not available >");
			// Draw rectangle
			CRect rect(usedWidth,
					 y,
					 usedWidth + printColWidth + colMarginX,
					 y + m_nLineHeight + colMarginY
					 );
			rect.right++; rect.bottom++;
			pDC->Rectangle(rect);
			rect.right--; rect.bottom--;

			// Draw text
			rect.DeflateRect(colMarginX / 2, colMarginY / 2);
			UINT nFormat = DT_VCENTER | DT_SINGLELINE;
			int horzAlign = (printAlign & (LVCFMT_RIGHT | LVCFMT_CENTER | LVCFMT_LEFT));
			switch (horzAlign) 
			{
			case LVCFMT_RIGHT:
				nFormat |= DT_RIGHT;
				break;
			case LVCFMT_CENTER:
				nFormat |= DT_CENTER;
				break;
			case LVCFMT_LEFT:
				nFormat |= DT_LEFT;
				break;
			default:
				ASSERT(FALSE);
				nFormat |= DT_LEFT;
				break;
			}
			pDC->DrawText(cs, rect, nFormat);

			// Update loop variables
			usedWidth += printColWidth + colMarginX;
		}
		y += m_nLineHeight + colMarginY;
	}
}

void CvSqlQueryLowerView::PrintNonSelect(CDC* pDC, int pageNum, int x, int &y, CaQueryNonSelectPageData* pPageData, int pageDataHeight)
{
	CString csTuplesCapt;
	ASSERT (pPageData->IsKindOf(RUNTIME_CLASS(CaQueryNonSelectPageData)));

	// caption
	pDC->SelectObject(&m_printerCaptFont);
	csTuplesCapt.LoadString(IDS_NON_SELECT_STATEMENT);
	pDC->TextOut(x, y, csTuplesCapt);
	y += m_nCaptLineHeight;
	y += m_nCaptLineHeight/2;
	pDC->SelectObject(&m_printerFont);
	y += m_nLineHeight; // Extra blank line
}

void CvSqlQueryLowerView::PrintQep(CDC* pDC, int pageNum, int x, int &y, CaQueryQepPageData* pPageData, int pageDataHeight)
{
	ASSERT (pPageData->IsKindOf(RUNTIME_CLASS(CaQueryQepPageData)));

	// caption
	pDC->SelectObject(&m_printerCaptFont);
	CString csTuplesCapt = _T("Query Execution Plan cannot be printed with this version.");
	pDC->TextOut(x, y, csTuplesCapt);
	y += m_nCaptLineHeight;
	y += m_nCaptLineHeight/2;
	pDC->SelectObject(&m_printerFont);
	y += m_nLineHeight; // Extra blank line
}

void CvSqlQueryLowerView::PrintRaw(CDC* pDC, int pageNum, int x, int &y, CaQueryRawPageData* pPageData, int pageDataHeight)
{
	CString csTuplesCapt;
	ASSERT (pPageData->IsKindOf(RUNTIME_CLASS(CaQueryRawPageData)));

	// caption
	pDC->SelectObject(&m_printerCaptFont);
	csTuplesCapt.LoadString (IDS_TRACE_OUTPUT);
	pDC->TextOut(x, y, csTuplesCapt);
	y += m_nCaptLineHeight;
	y += m_nCaptLineHeight/2;
	pDC->SelectObject(&m_printerFont);
	y += m_nLineHeight; // Extra blank line

	MultilineTextOut(pDC, x, y, pPageData->m_strTrace, pageDataHeight); // updates y
}

void CvSqlQueryLowerView::MultilineTextOut(CDC* pDC, int x, int& y, CString& rString, int pageDataHeight)
{
#define MULTILINE_CALCRECT
	// This Code takes advantage of DrawText() using DT_CALCRECT, ...
	// ...but does not wrap correctly if end of page reached.
#ifdef MULTILINE_CALCRECT
	ASSERT (x==0);
	CRect calcRect(x, y, m_nPageWidth, y + m_nLineHeight);
	pDC->DrawText(rString, calcRect, DT_EXTERNALLEADING | DT_WORDBREAK | DT_CALCRECT);
	int heightNeeded = calcRect.Height();
	ASSERT (heightNeeded % m_nLineHeight == 0);

	// Specific adjustment if would overlap to next page

	// Check line to be printed does not overlap 2 pages, and adjust if necessary
	int curLinePage = y / pageDataHeight;
	int nextLinePage = (y + heightNeeded) / pageDataHeight;
	ASSERT (nextLinePage >= curLinePage);
	if (nextLinePage > curLinePage) {
		int margin = (pageDataHeight - y) % m_nLineHeight;
		if (margin > 0) {
			y += margin;
			calcRect.OffsetRect(0, margin );
		}
		ASSERT ((pageDataHeight - y) % m_nLineHeight == 0);
	}
	pDC->DrawText(rString, calcRect, DT_EXTERNALLEADING | DT_WORDBREAK);
	y += heightNeeded;
#endif  // MULTILINE_CALCRECT

	// Alternate: this code wraps correctly when end of page reached,...
	// ...but truncates lines if too much characters between crlf
	// break line into pieces
#ifdef MULTILINE_CRLF
	TCHAR crlf[3];
	crlf[0] = 0x0d;
	crlf[1] = 0x0a;
	crlf[2] = 0;

	// Duplicate received string - Mandatory
	CString csMultilineTxt = rString;

	do {
		// search for 0d0a
		int index = csMultilineTxt.Find(crlf);
		if (index != -1) {
			ASSERT (index >= 0);

			// Check line to be printed does not overlap 2 pages, and adjust if necessary
			int curLinePage = y / pageDataHeight;
			int nextLinePage = (y + m_nLineHeight) / pageDataHeight;
			ASSERT (nextLinePage >= curLinePage);
			if (nextLinePage > curLinePage)
				y = nextLinePage * pageDataHeight;

			pDC->TextOut(x, y, csMultilineTxt, index);
			y += m_nLineHeight;
			int len = csMultilineTxt.GetLength();
			ASSERT (len - index - 2 >= 0 );
			csMultilineTxt = csMultilineTxt.Right(len - index - 2);
		}
		else {
			// last line

			// Check line to be printed does not overlap 2 pages, and adjust if necessary
			int curLinePage = y / pageDataHeight;
			int nextLinePage = (y + m_nLineHeight) / pageDataHeight;
			ASSERT (nextLinePage >= curLinePage);
			if (nextLinePage > curLinePage)
				y = nextLinePage * pageDataHeight;

			pDC->TextOut(x, y, csMultilineTxt);
			y += m_nLineHeight;
			break;  // break the do while
		}
	} while (!csMultilineTxt.IsEmpty());
#endif  // MULTILINE_CRLF
}












void CvSqlQueryLowerView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CView::OnEndPrintPreview(pDC, pInfo, point, pView);
}

