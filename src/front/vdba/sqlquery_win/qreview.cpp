/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qreview.cpp, Implementation file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : View window (CRichEditView that can format the text)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 21-Mar-2000 (noifr01)
**   (bug #100951) specified the SQL/Test session to keep its default 
**    isolation level
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qreview.h"
#include "qframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CvSqlQueryRichEditView, CRichEditView)

CvSqlQueryRichEditView::CvSqlQueryRichEditView()
{
	m_pLowerView = NULL;
}

CvSqlQueryRichEditView::~CvSqlQueryRichEditView()
{
}

void CvSqlQueryRichEditView::SetHighlight (BOOL bHighlight)
{
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	if (!pDoc)
		return;
	pDoc->m_bHighlight = bHighlight;
}

BOOL CvSqlQueryRichEditView::GetHighlight ()
{
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	if (!pDoc)
		return FALSE;
	return pDoc->m_bHighlight;
}



BEGIN_MESSAGE_MAP(CvSqlQueryRichEditView, CRichEditView)
	//{{AFX_MSG_MAP(CvSqlQueryRichEditView)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING, OnChangeSetting)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryRichEditView diagnostics

#ifdef _DEBUG
void CvSqlQueryRichEditView::AssertValid() const
{
	CRichEditView::AssertValid();
}

void CvSqlQueryRichEditView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvSqlQueryRichEditView message handlers

BOOL CvSqlQueryRichEditView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= ES_WANTRETURN|ES_MULTILINE|WS_VSCROLL;
	return CRichEditView::PreCreateWindow(cs);
}

void CvSqlQueryRichEditView::OnChange() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CRichEditView::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	if (!pDoc)
		return;

	if (pDoc->m_bHighlight)
	{
		CRichEditCtrl& ctrl = GetRichEditCtrl();
		ctrl.GetSel (pDoc->m_nStart, pDoc->m_nEnd);
		ctrl.SetSel (0, -1);
		OnColorPick (pDoc->m_crColorText);
		ctrl.SetSel (pDoc->m_nStart, pDoc->m_nEnd);
		pDoc->m_bHighlight = FALSE;
	}
	if (!m_pLowerView)
	{
		CSplitterWnd* pP1 = (CSplitterWnd*)GetParent(); // CSplitterWnd;
		ASSERT_VALID (pP1);
		m_pLowerView = (CvSqlQueryLowerView*)pP1->GetPane (1, 0);
	}
	if (m_pLowerView)
		m_pLowerView->GetDlgSqlQueryResult()->SendMessage (WM_SQL_STATEMENT_CHANGE, 0, 0);
}

void CvSqlQueryRichEditView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRichEditView::OnLButtonDown(nFlags, point);
}






void CvSqlQueryRichEditView::OnDestroy() 
{
	CRichEditView::OnDestroy();
}

void CvSqlQueryRichEditView::SetColor  (LONG nStart, LONG nEnd, COLORREF crColor)
{
	CRichEditCtrl& ctrl = GetRichEditCtrl();
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	pDoc->m_nStart = nStart;
	pDoc->m_nEnd   = nEnd;

	LONG oldEventMask = ctrl.SetEventMask(ENM_NONE);
	if (nStart == -1)
	{
		ctrl.SetSel (0, -1);
		OnColorPick (crColor);
		SetHighlight (FALSE);
		ctrl.SetSel (-1, 0);
	}
	else
	{
		ctrl.SetSel (-1, 0);
		ctrl.SetSel (nStart, nEnd);
		OnColorPick (crColor);
		SetHighlight (TRUE);
		ctrl.SetSel (-1, 0);
	}
	ctrl.SetEventMask(oldEventMask);
}

void CvSqlQueryRichEditView::OnInitialUpdate() 
{
	CRichEditView::OnInitialUpdate();
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	if (hFont)
		SendMessage(WM_SETFONT, (WPARAM)hFont);

	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	CfSqlQueryFrame* pFrame  = (CfSqlQueryFrame*)GetParentFrame();
	CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
	CaSqlQueryProperty& property = pDoc->GetProperty();
	if (pDlgResult && property.IsTraceActivated() && property.IsTraceToTop())
	{
		pDlgResult->SelectRawPage();
	}
}

void CvSqlQueryRichEditView::OnEditPaste() 
{
	CRichEditCtrl& ctrl = GetRichEditCtrl();
	if (ctrl.CanPaste(CF_TEXT))
		CRichEditView::OnEditPaste();
	else
	if (ctrl.CanPaste(CF_UNICODETEXT))
		CRichEditView::OnEditPaste();
	else
	if (ctrl.CanPaste(CF_OEMTEXT))
		CRichEditView::OnEditPaste();
	else
		MessageBeep (0xFFFFFFFF);
}

HRESULT CvSqlQueryRichEditView::QueryAcceptData(LPDATAOBJECT lpdataobj, CLIPFORMAT* lpcfFormat, DWORD dwReco, BOOL bReally, HGLOBAL hMetaPict)
{
	ASSERT(lpcfFormat != NULL);
	if (!bReally) // not actually pasting
		return S_OK;
	// if direct pasting a particular native format allow it
	if (IsRichEditFormat(*lpcfFormat))
		return S_OK;

	COleDataObject dataobj;
	dataobj.Attach(lpdataobj, FALSE);
	// if format is 0, then force particular formats if available
	if (*lpcfFormat == 0 && (m_nPasteType == 0))
	{
		if (dataobj.IsDataAvailable(CF_TEXT))
		{
			*lpcfFormat = CF_TEXT;
			return S_OK;
		}
	}
	return S_FALSE;
}

void CvSqlQueryRichEditView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_F5)
	{
		CfSqlQueryFrame* pFrame  = (CfSqlQueryFrame*)GetParentFrame();
		if (pFrame && pFrame->IsRunEnable())
			pFrame->Execute();
	}

	CRichEditView::OnKeyDown(nChar, nRepCnt, nFlags);
}


#define COMMIT_AFFIRMATION_IS_TRUE
void CvSqlQueryRichEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	switch (nHint)
	{
	case UPDATE_HINT_LOAD:
		if (pHint && pHint == pDoc)
		{
			// Load document from the serialization:
			if (!pDoc->IsLoadedDoc())
				return;
			//
			// Put the global statement.
			CRichEditCtrl& redit = GetRichEditCtrl();
			LONG oldEventMask = redit.SetEventMask(ENM_NONE); // Do not send EN_CHANGE.
			redit.SetWindowText (pDoc->m_strStatement);
			// Set the highlight statement.
			if (pDoc->m_bHighlight)
			{
				redit.SetSel (0, -1);
				OnColorPick (pDoc->m_crColorText);
				redit.SetSel (pDoc->m_nStart, pDoc->m_nEnd);
				OnColorPick (pDoc->m_crColorSelect);
				redit.SetSel (-1, 0);
			}
			redit.SetEventMask(oldEventMask);// Restore the old event mask
			pDoc->SetTitle (pDoc->m_strTitle);
			//
			// Splitbar placement
			CfSqlQueryFrame* pFrame  = (CfSqlQueryFrame*)GetParentFrame();
			CSplitterWnd* pNestSplitter = pFrame->GetNestSplitterWnd();
			ASSERT (pNestSplitter); 
			CSplitterWnd* pMainSplitter = pFrame->GetMainSplitterWnd();
			ASSERT (pMainSplitter); 
			if (!(pMainSplitter && pNestSplitter))
				return;

			pMainSplitter->SetRowInfo(0, pDoc->GetSplitterCyCurMainSplit(), pDoc->GetSplitterCyMinMainSplit());
			pMainSplitter->RecalcLayout();
			pNestSplitter->SetRowInfo(0, pDoc->GetSplitterCyCurNestSplit(), pDoc->GetSplitterCyMinNestSplit());
			pNestSplitter->RecalcLayout();
		}
		break;
	case UPDATE_HINT_SETTINGCHANGE:
/*UKS
		if (pHint && pHint == pDoc && pDoc->GetSetting().GetFont())
		{
			long nStart;
			long nEnd;
			GetRichEditCtrl().GetSel(nStart, nEnd);
			//
			// The control simply uses the WM_SETFONT message to change the font.
			// There is no need to use the member:
			//    SetSelectionCharFormat,
			//    SetDefaultCharFormat,
			//    SetParaFormat,
			//    SetCharFormat,
			// because the control does not manage the text in different font.
			CString strText;
			GetRichEditCtrl().GetWindowText (strText);
			LONG oldEventMask = GetRichEditCtrl().SetEventMask(ENM_NONE); // Do not send EN_CHANGE.
			GetRichEditCtrl().Clear();
			SendMessage(WM_SETFONT, (WPARAM)pDoc->GetSetting().GetFont());
			Invalidate();
			GetRichEditCtrl().SetWindowText(strText);
			if (pDoc->m_bHighlight)
			{
				GetRichEditCtrl().SetSel (pDoc->m_nStart, pDoc->m_nEnd);
				OnColorPick (pDoc->m_crColorSelect);
				GetRichEditCtrl().SetSel (nStart, nEnd);
			}
			GetRichEditCtrl().SetEventMask(oldEventMask); 
		}
*/
		break;

	default:
		break;
	}
}

long CvSqlQueryRichEditView::OnChangeSetting(WPARAM wParam, LPARAM lParam)
{
	int nWhat = (int)wParam;
	CaSqlQueryProperty* pSetting = (CaSqlQueryProperty*)lParam;
	if (!pSetting)
		return 0;
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)GetDocument();
	if (!pDoc)
		return 0;

	if (nWhat & SQLMASK_FONT)
	{
		HFONT fFont = (HFONT)pSetting->GetFont();
		long nStart;
		long nEnd;
		GetRichEditCtrl().GetSel(nStart, nEnd);
		//
		// The control simply uses the WM_SETFONT message to change the font.
		// There is no need to use the member:
		//    SetSelectionCharFormat,
		//    SetDefaultCharFormat,
		//    SetParaFormat,
		//    SetCharFormat,
		// because the control does not manage the text in different font.
		CString strText;
		GetRichEditCtrl().GetWindowText (strText);
		LONG oldEventMask = GetRichEditCtrl().SetEventMask(ENM_NONE); // Do not send EN_CHANGE.
		GetRichEditCtrl().Clear();
		SendMessage (WM_SETFONT, (WPARAM)fFont, MAKELPARAM(TRUE, 0));
		Invalidate();
		GetRichEditCtrl().SetWindowText(strText);
		if (pDoc->m_bHighlight)
		{
			GetRichEditCtrl().SetSel (pDoc->m_nStart, pDoc->m_nEnd);
			OnColorPick (pDoc->m_crColorSelect);
			GetRichEditCtrl().SetSel (nStart, nEnd);
		}
		GetRichEditCtrl().SetEventMask(oldEventMask); 
	}
	return 0;
}


void CvSqlQueryRichEditView::OnEditCopy() 
{
	GetRichEditCtrl().Copy();
}


void CvSqlQueryRichEditView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CRichEditView::OnRButtonDown(nFlags, point);
	CPoint p = point;
	ClientToScreen(&p);
	CRichEditCtrl& ctrl = GetRichEditCtrl();

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_RICHTEXT_POPUP));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;

	long nStartChar= -1, nEndChar = -1;
	GetRichEditCtrl().GetSel(nStartChar, nEndChar);
	UINT nFlagCutCopy = (nStartChar != nEndChar)? MF_ENABLED: MF_DISABLED|MF_GRAYED;
	UINT nFlagPaste   = ::IsClipboardFormatAvailable(CF_TEXT)? MF_ENABLED: MF_DISABLED|MF_GRAYED; 
	UINT nFlagUndo   = ctrl.CanUndo()? MF_ENABLED: MF_DISABLED|MF_GRAYED; 

	pPopup->EnableMenuItem(ID_EDIT_COPY, MF_BYCOMMAND|nFlagCutCopy);
	pPopup->EnableMenuItem(ID_EDIT_CUT,  MF_BYCOMMAND|nFlagCutCopy);
	pPopup->EnableMenuItem(ID_EDIT_PASTE,  MF_BYCOMMAND|nFlagPaste);
	pPopup->EnableMenuItem(ID_EDIT_UNDO,  MF_BYCOMMAND|nFlagUndo);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
}

void CvSqlQueryRichEditView::OnEditCut() 
{
	GetRichEditCtrl().Cut();
}

void CvSqlQueryRichEditView::OnEditUndo() 
{
	GetRichEditCtrl().Undo();
}
