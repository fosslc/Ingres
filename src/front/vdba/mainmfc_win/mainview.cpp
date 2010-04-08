// MainView.cpp : implementation of the CMainMfcView class
//
/*
** History:
**
**
** 19-Mar-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up warnings.
**
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"

#include "maindoc.h"
#include "mainview.h"
#include "childfrm.h"

#include "toolbars.h"

#include "cmixfile.h"

#include "cmdline.h"    // For unicenter driven stuff

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dba.h"
#include "main.h"
#include "dom.h"
#include "dbafile.h"    // MakeNewDomMDIChild

// dom.c
//BOOL DomOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
//void DomOnSize(HWND hwnd, UINT state, int cx, int cy);
//void DomOnDestroy(HWND hwnd);
//void DomOnMdiActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);

typedef struct  _sDomCreateData
{
  int       domCreateMode;                // dom creation mode: tearout,...
  HWND      hwndMdiRef;                   // referred mdi dom doc
  FILEIDENT idFile;                       // if loading: file id to read from
  LPDOMNODE lpsDomNode;                   // node created from previous name
} DOMCREATEDATA, FAR *LPDOMCREATEDATA;

extern DOMCREATEDATA globDomCreateData;

};

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView

IMPLEMENT_DYNCREATE(CMainMfcView, CView)

BEGIN_MESSAGE_MAP(CMainMfcView, CView)
	//{{AFX_MSG_MAP(CMainMfcView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView construction/destruction

CMainMfcView::CMainMfcView()
{
	// TODO: add construction code here

}

CMainMfcView::~CMainMfcView()
{
}

BOOL CMainMfcView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
  cs.lpszClass = szDomMDIChild;
  cs.style |= WS_CLIPCHILDREN;  // Otherwise gray effect!
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView drawing

void CMainMfcView::OnDraw(CDC* pDC)
{
	CMainMfcDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView printing

BOOL CMainMfcView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CMainMfcView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CMainMfcView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView diagnostics

#ifdef _DEBUG
void CMainMfcView::AssertValid() const
{
	CView::AssertValid();
}

void CMainMfcView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMainMfcDoc* CMainMfcView::GetDocument() // non-debug version is inline
{
	if ( m_pDocument) {
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMainMfcDoc)));
	}
	return (CMainMfcDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView message handlers


int CMainMfcView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  // prepare structure for call to DomOnCreate()
  LPMDICREATESTRUCT lpMcs = (LPMDICREATESTRUCT) lpCreateStruct->lpCreateParams;
  lpMcs->lParam = (long)&globDomCreateData;

	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}


void CMainMfcView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
  //if (pActivateView != pDeactiveView)
  //  DomOnMdiActivate(m_hWnd,
  //                   bActivate,
  //                   pActivateView!=0 ? pActivateView->m_hWnd : 0,
  //                   pDeactiveView!=0 ? pDeactiveView->m_hWnd : 0);

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CMainMfcView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

  CMainMfcDoc* pDoc = (CMainMfcDoc *)GetDocument();
  ASSERT(pDoc);
  CChildFrame *pFrame = (CChildFrame*)GetParent()->GetParent();  // 2 levels since splitter wnd

  // reload document serialization information
  if (globDomCreateData.domCreateMode == DOM_CREATE_LOAD) {
    // load document data from archive file
    CMixFile *pMixFile = GetCMixFile();
    CArchive ar(pMixFile, CArchive::load, ARCHIVEBUFSIZE);
    try {
      DWORD dw = (DWORD)pMixFile->GetPosition();
      ASSERT (dw);
      pDoc->Serialize(ar);
      ar.Close();
    }
    // a file exception occured
    catch (CFileException* pFileException)
    {
      pFileException->Delete();
      ar.Close();
      return;
    }
    // an archive exception occured
    catch (CArchiveException* pArchiveException)
    {
      VDBA_ArchiveExceptionMessage(pArchiveException);
      pArchiveException->Delete();
      return;
    }
  }

  if (pDoc->IsLoadedDoc()) {
    // frame window placement
    BOOL bResult = pFrame->SetWindowPlacement(pDoc->GetWPLJ());
    ASSERT (bResult);

    // set full state of all toolbars in the frame, including visibility
    CDockState& ToolbarsState = pDoc->GetToolbarState();
    pFrame->SetDockState(ToolbarsState);
    pFrame->RecalcLayout();

    /* found useless emb Dec. 9, 97: managed by SetDockState()
    // Set frame toolbar visibility state according to load info
    if (pDoc->m_bToolbarVisible)
      SetToolbarVisible(pFrame, TRUE);      // force immediate update
    else
      SetToolbarInvisible(pFrame, TRUE);    // force immediate update
    */

    // splitbar placement - after toolbars state restore
    int cxCur = pDoc->GetSplitterCxCur();
    int cxMin = pDoc->GetSplitterCxMin();
    CSplitterWnd *pSplit = (CSplitterWnd *)GetParent();
    ASSERT (pSplit);
    pSplit->SetColumnInfo(0, cxCur, cxMin);
    pSplit->RecalcLayout();

  }
  else {
    // default splitbar positioning, except if unicenter driven with specified positioning
    BOOL bUseDefault = TRUE;
    if (IsUnicenterDriven()) {
      CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
      ASSERT (pDescriptor);
      ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
      if (pDescriptor->HasSplitbarPercentage())
        bUseDefault = FALSE;
    }
    if (bUseDefault) {
      CRect rcClient;
      CSplitterWnd *pSplit = (CSplitterWnd *)GetParent();
      ASSERT (pSplit);
      CWnd* pFrame = pSplit->GetParent();
      ASSERT (pFrame);
      pFrame->GetClientRect (rcClient);
      pSplit->SetColumnInfo(0, (int) (0.4 * (double)rcClient.Width()), 10);
      pSplit->RecalcLayout();
    }
  }
}

