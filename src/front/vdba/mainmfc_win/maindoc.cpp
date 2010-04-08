// MainDoc.cpp : implementation of the CMainMfcDoc class
//

#include "stdafx.h"

#include "mainmfc.h"
#include "maindoc.h"
#include "childfrm.h"

#include "toolbars.h"

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
#include "domdata.h"
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcDoc

IMPLEMENT_DYNCREATE(CMainMfcDoc, CDocument)

BEGIN_MESSAGE_MAP(CMainMfcDoc, CDocument)
	//{{AFX_MSG_MAP(CMainMfcDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainMfcDoc construction/destruction

CMainMfcDoc::CMainMfcDoc()
{
  // default states
  m_bToolbarVisible = TRUE;

  // serialization
  m_bLoaded = FALSE;

  // tab control in right pane
  m_pTabDialog  = NULL;

  // current property page
  m_pCurrentProperty = NULL;

  // Filter structure
  memset (&m_sFilter, 0, sizeof(SFILTER));

}

CMainMfcDoc::~CMainMfcDoc()
{
  if (m_pCurrentProperty)
    delete m_pCurrentProperty;
}

BOOL CMainMfcDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcDoc serialization

void CMainMfcDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())	{
    // filter structure
    ar.Write(&m_sFilter, sizeof(m_sFilter));

    // visibility state of the related frame toolbar
    POSITION pos = GetFirstViewPosition();
    CView *pView = GetNextView(pos);
    ASSERT (pView);
    CChildFrame *pFrame = (CChildFrame*)pView->GetParent()->GetParent();  // 2 levels since splitter wnd
    ASSERT (pFrame);
    if(IsToolbarVisible(pFrame))
      ar << TRUE;
    else
      ar << FALSE;

    // full state of all toolbars in the frame
    pFrame->GetDockState(m_toolbarState);
    m_toolbarState.Serialize(ar);

    // frame window placement
    memset(&m_wplj, 0, sizeof(m_wplj));
    BOOL bResult = pFrame->GetWindowPlacement(&m_wplj);
    ASSERT (bResult);
    ar.Write(&m_wplj, sizeof(m_wplj));

    // splitbar
    CSplitterWnd *pSplit = (CSplitterWnd *)pView->GetParent();
    ASSERT (pSplit);
    pSplit->GetColumnInfo(0, m_cxCur, m_cxMin);
    ar << m_cxCur;
    ar << m_cxMin;

    //
    // Data manipulate by DOM Page (Modeless Dialog)
    try
    {
        ASSERT (m_pTabDialog);
        CuIpmPropertyData* pData = m_pTabDialog->GetDialogData();
        CuDomProperty* pCurrentProperty = m_pTabDialog->GetCurrentProperty();
        if (pCurrentProperty) 
           pCurrentProperty->SetPropertyData (pData);
        ar << pCurrentProperty;
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }

  }
	else {
    m_bLoaded = TRUE;

    // filter structure
    ar.Read(&m_sFilter, sizeof(m_sFilter));

    // visibility state of the related frame toolbar
    ar >> m_bToolbarVisible;

    // full state of all toolbars in the frame
    m_toolbarState.Serialize(ar);

    // frame window placement
    ar.Read(&m_wplj, sizeof(m_wplj));

    // splitbar
    ar >> m_cxCur;
    ar >> m_cxMin;

    //
    // Data manipulate by DOM Page (Modeless Dialog)
    try
    {
        if (m_pCurrentProperty)
        {
            delete m_pCurrentProperty;
            m_pCurrentProperty = NULL;
        }
        ar >> m_pCurrentProperty;
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }

  }
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcDoc diagnostics

#ifdef _DEBUG
void CMainMfcDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMainMfcDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainMfcDoc commands
IUnknown* CMainMfcDoc::GetSqlUnknown()
{
	CuDlgDomTabCtrl* pTab = GetTabDialog();
	ASSERT(pTab);
	if (!pTab)
		return NULL;
	CWnd* pWndTbRow = pTab->GetCreatedPage(IDD_DOMPROP_TABLE_ROWS);
	if (!pWndTbRow)
		return NULL;
	//
	// wParam = 1 (request for the sql control)
	return (IUnknown*)pWndTbRow->SendMessage(WM_USER_IPMPAGE_GETDATA, (WPARAM)1);
}

