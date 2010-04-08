/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DocVNode.cpp, Implementation file    (Document for MDI Child Frame)   //
//                                                     CFrmVNodeMDIChild               //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when            //
//               it is not a Docking View.                                             //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "docvnode.h"
#include "frmvnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDocVNodeMDIChild, CDocument)

CDocVNodeMDIChild::CDocVNodeMDIChild()
{
  // serialization
  m_bLoaded = FALSE;
}

BOOL CDocVNodeMDIChild::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    return TRUE;
}

CDocVNodeMDIChild::~CDocVNodeMDIChild()
{
}


BEGIN_MESSAGE_MAP(CDocVNodeMDIChild, CDocument)
    //{{AFX_MSG_MAP(CDocVNodeMDIChild)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDocVNodeMDIChild diagnostics

#ifdef _DEBUG
void CDocVNodeMDIChild::AssertValid() const
{
    CDocument::AssertValid();
}

void CDocVNodeMDIChild::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDocVNodeMDIChild serialization

void CDocVNodeMDIChild::Serialize(CArchive& ar)
{
  if (ar.IsStoring()) {
    POSITION pos = GetFirstViewPosition();
    CView *pView = GetNextView(pos);
    ASSERT (pView);
    CFrmVNodeMDIChild *pFrame = (CFrmVNodeMDIChild*)pView->GetParent();   // 1 level since no splitter wnd
    ASSERT (pFrame);

    // frame window placement
    memset(&m_wplj, 0, sizeof(m_wplj));
    BOOL bResult = pFrame->GetWindowPlacement(&m_wplj);
    ASSERT (bResult);
    ar.Write(&m_wplj, sizeof(m_wplj));
  }
  else {
    m_bLoaded = TRUE;

    // frame window placement
    ar.Read(&m_wplj, sizeof(m_wplj));
  }
}

/////////////////////////////////////////////////////////////////////////////
// CDocVNodeMDIChild commands
