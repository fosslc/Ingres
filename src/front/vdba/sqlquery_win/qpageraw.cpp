/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageraw.cpp, implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains the result from execution of raw statement.
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
** 04-Jun-2002 (hanje04)
**     Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**     unwanted ^M's in generated text files.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qframe.h"
#include "qpageraw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSqlQueryPageRaw::CuDlgSqlQueryPageRaw(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageRaw::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSqlQueryPageRaw)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_strCallerID = _T("");
	m_nLine    = 0;
	m_nLength  = 0;
	m_bLimitReached = FALSE; 
}


void CuDlgSqlQueryPageRaw::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageRaw)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageRaw, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageRaw)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_PAGE_HIGHLIGHT,  OnHighlightStatement)
	ON_MESSAGE (WM_SQL_STATEMENT_SHOWRESULT,  OnDisplayResult)
	ON_MESSAGE (WM_SQL_GETPAGE_DATA,          OnGetData)
	ON_MESSAGE (WM_SQL_TAB_BITMAP,            OnGetTabBitmap)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,      OnChangeSetting)
	ON_MESSAGE (WMUSRMSG_GETFONT,             OnGetSqlFont)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgSqlQueryPageRaw message handlers


void CuDlgSqlQueryPageRaw::NotifyLoad (CaQueryRawPageData* pData)
{
	m_strEdit1 = pData->m_strTrace;
	m_nLine    =   pData->m_nLine;
	m_nLength  = pData->m_nLength;
	m_bLimitReached = pData->m_bLimitReached;
	m_cEdit1.SetWindowText (pData->m_strTrace);
}


LONG CuDlgSqlQueryPageRaw::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaQueryRawPageData* pData    = NULL;
	try
	{
		pData = new CaQueryRawPageData();
		m_cEdit1.GetWindowText (pData->m_strTrace);
		pData->m_nLine         = m_nLine;
		pData->m_nLength       = m_nLength;
		pData->m_bLimitReached = m_bLimitReached;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return (LONG)NULL;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_GETDATA_FOR_SAVE, MB_ICONEXCLAMATION|MB_OK);
		if (pData)
			delete pData;
		return (LONG)NULL;
	}
	return (LONG)pData;
}


LONG CuDlgSqlQueryPageRaw::OnDisplayResult (WPARAM wParam, LPARAM lParam)
{
	return (LONG)0L;
}


void CuDlgSqlQueryPageRaw::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgSqlQueryPageRaw::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return TRUE;
	CvSqlQueryRichEditView* pRichEditView = pFrame->GetRichEditView();
	ASSERT (pRichEditView);
	if (!pRichEditView)
		return TRUE;
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pRichEditView->GetDocument();
	ASSERT (pDoc);
	if (pDoc)
	{
		HFONT hf = pDoc->GetProperty().GetFont();
		if (hf)
			m_cEdit1.SendMessage(WM_SETFONT, (WPARAM)hf);
	}
	m_cEdit1.LimitText();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageRaw::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect r;
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	GetClientRect (r);
	m_cEdit1.MoveWindow (r);
}

LONG CuDlgSqlQueryPageRaw::OnGetTabBitmap (WPARAM wParam, LPARAM lParam)
{
	return (LONG)BM_TRACE;
}


LONG CuDlgSqlQueryPageRaw::OnHighlightStatement (WPARAM wParam, LPARAM lParam)
{
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return 0;
	CvSqlQueryRichEditView* pRichEditView = pFrame->GetRichEditView();
	ASSERT (pRichEditView);
	if (!pRichEditView)
		return 0;
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pRichEditView->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return 0L;

	pRichEditView->SetColor (-1,  0, pDoc->m_crColorText);
	return 0L;
}

void CuDlgSqlQueryPageRaw::DisplayTraceLine (LPCTSTR strStatement, LPCTSTR lpszTrace, LPCTSTR lpszCursorName)
{
	TCHAR tchszBuffLine [512];
#ifdef MAINWIN
	TCHAR tchszReturn[] = {0x0A, 0x00};
#else
	TCHAR tchszReturn[] = {0x0D, 0x0A, 0x00};
#endif
	CString strMsgNoAvailable;
	strMsgNoAvailable.LoadString(IDS_PREVIOUS_LINE_UNAVAILABLE);
	int nMaxBytes = 0;       // Maximum bytes of text allowed by the edit
	int nNewBytes = 0;       // Number of bytes for the new text to be added
	int nCurrentBytes = 0;   // Number of bytes of the current text in the edit.
	int nLines = 0;          // How many lines to destroy (from the first) to get at least nNewByte;
	int nSize, nLen = m_cEdit1.GetWindowTextLength();
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pFrame->GetActiveDocument();
	strMsgNoAvailable += tchszReturn;
	CaSqlQueryProperty& property = pDoc->GetProperty();

	CString strLine;
	if (strStatement)
	{
		strLine  = strStatement;
		strLine += tchszReturn;

		nNewBytes = strLine.GetLength(); 
		nCurrentBytes = m_cEdit1.GetWindowTextLength() * sizeof (TCHAR);
		nMaxBytes = property.GetMaxTrace() * 1024 + strMsgNoAvailable.GetLength();

		if (nNewBytes + nCurrentBytes >= nMaxBytes)
		{
			if (!m_bLimitReached)
			{
				m_bLimitReached = TRUE;
				AfxMessageBox (IDS_MSG_SIZE_BUFFER_REACHED); 
			}
			m_cEdit1.Invalidate();
			nLines = 1;
			nSize  = 0;
			int nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
			nSize += nLineSize +2; // +2 (for the tchszReturn)
			while (nSize <= nNewBytes)
			{
				nLines++;
				nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
				nSize += nLineSize+2; // +2 (for the tchszReturn)
			}
			m_cEdit1.SetSel (0, nSize);
			m_cEdit1.ReplaceSel (strMsgNoAvailable);
			m_cEdit1.Invalidate();
		}
		nLen = m_cEdit1.GetWindowTextLength();
		m_cEdit1.SetSel (nLen, nLen);
		m_cEdit1.ReplaceSel (strLine);
		m_nLine++;
	}
	if (lpszTrace)
	{
		strLine  = lpszTrace;
		strLine += tchszReturn;
		nNewBytes = strLine.GetLength(); 
		nCurrentBytes = m_cEdit1.GetWindowTextLength() * sizeof (TCHAR);
		nMaxBytes = property.GetMaxTrace() * 1024;  // (pSetting->m_nTraceBuffer is in KBytes)

		if (nNewBytes + nCurrentBytes >= nMaxBytes)
		{
			if (!m_bLimitReached)
			{
				m_bLimitReached = TRUE;
				//
				// MSG= The maximum Trace Tab size (defined in the preferences) has been reached.\n
				//      The first lines in the Trace Tab will be removed each time new lines will be added at the end.
				AfxMessageBox (IDS_MSG_TRACE_TAB_SIZE_REACHED); 
			}
			m_cEdit1.Invalidate();
			nLines = 1;
			nSize  = 0;
			int nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
			nSize += nLineSize +2; // +2 (for the tchszReturn)
			while (nSize <= nNewBytes)
			{
				nLines++;
				nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
				nSize += nLineSize +2; // +2 (for the tchszReturn)
			}
			m_cEdit1.SetSel (0, nSize);
			m_cEdit1.ReplaceSel (strMsgNoAvailable);
			m_cEdit1.Invalidate();
		}
		nLen = m_cEdit1.GetWindowTextLength();
		m_cEdit1.SetSel (nLen, nLen);
		m_cEdit1.ReplaceSel (strLine);
		m_nLine++;
	}
	m_strCallerID = lpszCursorName;
	m_nLength = m_cEdit1.GetWindowTextLength();
}


LONG CuDlgSqlQueryPageRaw::OnChangeSetting (WPARAM wParam, LPARAM lParam)
{
	UINT nMask = (UINT)wParam;
	CaSqlQueryProperty* pProperty = (CaSqlQueryProperty*)lParam;
	if (!pProperty)
		return 0;
	if (nMask & SQLMASK_FONT)
	{
		m_cEdit1.SendMessage (WM_SETFONT, (WPARAM)pProperty->GetFont(), MAKELPARAM(TRUE, 0));
	}

	if (nMask & SQLMASK_MAXTRACESIZE)
	{
		int nLines = 0;  // How many lines to destroy (from the first) to get at least nNewByte;
		int nSize, nLen = 0;
		int nMaxBytes = pProperty->GetMaxTrace() * 1024;  // (pSetting->m_nTraceBuffer is in KBytes)
		int nCurrentBytes = m_cEdit1.GetWindowTextLength() * sizeof (TCHAR);
		if (nCurrentBytes <= nMaxBytes)
			return 0;
		TCHAR tchszBuffLine [512];
		CString strMsgNoAvailable;
		strMsgNoAvailable.LoadString(IDS_PREVIOUS_LINE_UNAVAILABLE);//= _T("<Some previous lines are no more available>");
		nLines = 1;
		nSize  = 0;
		int nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
		nSize += nLineSize +2; // +2 (for the tchszReturn)
		while ((nCurrentBytes - nSize) > nMaxBytes)
		{
			nLines++;
			nLineSize = m_cEdit1.GetLine(nLines-1, tchszBuffLine, 512);
			nSize += nLineSize+2; // +2 (for the tchszReturn)
		}
		m_cEdit1.SetSel (0, nSize);
		m_cEdit1.ReplaceSel (strMsgNoAvailable);
		m_cEdit1.Invalidate();
	}
	return 0;
}

LONG CuDlgSqlQueryPageRaw::OnGetSqlFont    (WPARAM wParam, LPARAM lParam)
{
	HFONT hf = (HFONT)m_cEdit1.SendMessage(WM_GETFONT);
	return (LONG)hf;
}
