/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewrigh.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc) CView of Right Pane that contains the Dialogs
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 22-Dec-2000 (noifr01)
**    (SIR 103548) show the value of II_INSTALLATION in the title
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 11-Jun-2001
**    SIR #104918, If ijactrl.ocx is not registered then register it.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Use the aboutbox provided by ijactrl.ocx. and remove
**    the local about.bmp.
** 23-Sep-2004 (uk$so01)
**    BUG #113114 
**    Force the SendMessageToDescendants(WM_SYSCOLORCHANGE) to reflect 
**    immediately the system color changed.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "maindoc.h"
#include "libguids.h"
#include ".\viewrigh.h"

extern "C"
{
#include <compat.h>
#include <er.h>
#include <nm.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight = FW_NORMAL, int bItalic= FALSE);
static BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight, int bItalic)
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight        = size; 
	lf.lfWidth         = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = weight;
	lf.lfItalic        = bItalic;
	lf.lfUnderline     = 0;
	lf.lfStrikeOut     = 0;
	lf.lfCharSet       = 0;
	lf.lfOutPrecision  = OUT_CHARACTER_PRECIS;
	lf.lfClipPrecision = 2;
	lf.lfQuality       = PROOF_QUALITY;
	lstrcpy(lf.lfFaceName, lpFaceName);
	return font.CreateFontIndirect(&lf);
}

/////////////////////////////////////////////////////////////////////////////
// CvViewRight

IMPLEMENT_DYNCREATE(CvViewRight, CView)

CvViewRight::CvViewRight()
{
	m_pCtrl = NULL;

	char *ii_installation;
#ifdef EDBC
	NMgtAt( ERx( "ED_INSTALLATION" ), &ii_installation );
#else
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
#endif
	if( ii_installation == NULL || *ii_installation == EOS )
		m_csii_installation = _T(" [<error>]");
	else
		m_csii_installation.Format(_T(" [%s]"),ii_installation);
}

CvViewRight::~CvViewRight()
{
}


BEGIN_MESSAGE_MAP(CvViewRight, CView)
	//{{AFX_MSG_MAP(CvViewRight)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvViewRight drawing

void CvViewRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	CString strTitle; 
	CString strFormat =((CdMainDoc*)pDoc)->m_strFormat;
	strTitle.LoadString (AFX_IDS_APP_TITLE);
	strTitle += strFormat;
	strTitle+=m_csii_installation;

	AfxGetMainWnd()->SetWindowText((LPCTSTR)strTitle);
	if (!m_pCtrl)
	{
		CRect r;
		GetClientRect (r);
		CFont font;
		if (!UxCreateFont (font, _T("Arial"), 30))
			return;

		CString strMsg;
		if (!strMsg.LoadString(IDS_IJACONTROL_NOT_REGISTERED))
			strMsg = _T("IJA ActiveX Control is not Registered");
		CFont* pOldFond = pDC->SelectObject (&font);
		COLORREF colorTextOld   = pDC->SetTextColor (RGB (0, 0, 255));
		COLORREF oldTextBkColor = pDC->SetBkColor (GetSysColor (COLOR_WINDOW));
		CSize txSize= pDC->GetTextExtent (strMsg, strMsg.GetLength());
		int iPrevMode = pDC->SetBkMode (TRANSPARENT);
		pDC->DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		pDC->SetBkMode (iPrevMode);
		pDC->SelectObject (pOldFond);
		pDC->SetBkColor (oldTextBkColor);
		pDC->SetTextColor (colorTextOld);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CvViewRight diagnostics

#ifdef _DEBUG
void CvViewRight::AssertValid() const
{
	CView::AssertValid();
}

void CvViewRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvViewRight message handlers

int CvViewRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CRect r;
	GetClientRect (r);

	if (LIBGUIDS_IsCLSIDRegistered(_T("{C92B8427-B176-11D3-A322-00C04F1F754A}")))
	{
		m_pCtrl = new CIjaCtrl();
		m_pCtrl->Create(
			_T("IJACTRL"), 
			_T("IJAAPPLICATION"),
			WS_CHILD|WS_VISIBLE,
			r,
			this, 
			1001);
		m_pCtrl->UpdateWindow();
		m_pCtrl->ShowWindow(SW_SHOW);
		m_pCtrl->SetHelpFilePath(theApp.m_pszHelpFilePath);
		m_pCtrl->SetSessionStart(100);
	}

	return 0;
}

void CvViewRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_pCtrl && IsWindow (m_pCtrl->m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_pCtrl->MoveWindow (r);
		m_pCtrl->ShowWindow (SW_SHOW);
	}
}



void CvViewRight::PostNcDestroy() 
{
	delete m_pCtrl;
	CView::PostNcDestroy();
}

void CvViewRight::OnAbout() 
{
	if (m_pCtrl && IsWindow (m_pCtrl->m_hWnd))
		m_pCtrl->AboutBox();
}


void CvViewRight::OnSysColorChange()
{
	CView::OnSysColorChange();
	if (m_pCtrl && IsWindow (m_pCtrl->m_hWnd))
		m_pCtrl->SendMessageToDescendants(WM_SYSCOLORCHANGE);
}
