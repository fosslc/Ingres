/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.  		       //
//                                                                                     //
//    Source   : FrmVNode.cpp, Implementation file    (MDI Child Frame)                //
//                                                                                     //
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
#include "mainfrm.h"
#include "frmvnode.h"
#include "docvnode.h"
#include "vewvnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "main.h"       // WM_USER_GETNODEHANDLE
}

IMPLEMENT_DYNCREATE(CFrmVNodeMDIChild, CMDIChildWnd)

CFrmVNodeMDIChild::CFrmVNodeMDIChild()
{
}

CFrmVNodeMDIChild::~CFrmVNodeMDIChild()
{
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  ASSERT (pMainFrame);

  pMainFrame->SetPVirtualNodeMdiDoc(NULL);
  pMainFrame->SetNodesVisibilityFlag(FALSE);
}


BEGIN_MESSAGE_MAP(CFrmVNodeMDIChild, CMDIChildWnd)
    //{{AFX_MSG_MAP(CFrmVNodeMDIChild)
    ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
  ON_MESSAGE(WM_USER_GETNODEHANDLE, OnGetNodeHandle)
  ON_MESSAGE (WM_USER_GETMFCDOCTYPE, OnGetMfcDocType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFrmVNodeMDIChild message handlers
//#define UKS_SPEC
int CFrmVNodeMDIChild::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
#ifdef UKS_SPEC        
    if (!m_wndToolBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS) ||
        !m_wndToolBar.LoadToolBar(IDR_VNODEMDI2))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
#else
    if (!m_wndToolBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS) ||
        !m_wndToolBar.LoadToolBar(IDR_VNODEMDI))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
#endif
    return 0;
}


LONG CFrmVNodeMDIChild::OnGetNodeHandle(UINT uu, LONG ll)
{
  return -2L;   // Never on a node! - should not be -1L since confused with unconnected node!
}

LONG CFrmVNodeMDIChild::OnGetMfcDocType(UINT uu, LONG ll)
{
  CDocument* pDoc = GetActiveDocument();
  if (pDoc)
    return TYPEDOC_NODESEL;
  else
    return TYPEDOC_UNKNOWN;
}


void CFrmVNodeMDIChild::OnDestroy() 
{
  theApp.SetLastDocMaximizedState(IsZoomed());

	CMDIChildWnd::OnDestroy();
}
