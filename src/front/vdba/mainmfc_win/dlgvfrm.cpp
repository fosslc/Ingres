/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DlgVFrm.cpp, Implementation file.
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for CFormView, Scrollable Dialog box of Detail Page.
** 
** Histoty:
** xx-xx-1997 (uk$so01)
**    Created.
** 27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 20-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dlgvfrm.h"
#include "dlgvdoc.h"

// dom screens with scrollbars
#include "dgdploc.h"    // CuDlgDomPropLocation
#include "dgdpusr.h"    // CuDlgDomPropUser
#include "dgdprol.h"    // CuDlgDomPropRole
#include "dgdppro.h"    // CuDlgDomPropProfile
#include "dgdpidx.h"    // CuDlgDomPropIndex
#include "dgdpprc.h"    // CuDlgDomPropProcedure (Prc)
#include "dgdpdb.h"     // CuDlgDomPropDb
#include "dgdptb.h"     // CuDlgDomPropTable
#include "dgdpco.h"     // CuDlgDomPropConnection (replicator)
#include "dgdpdd.h"     // CuDlgDomPropDDb

#include "dgdpdpl.h"    // CuDlgDomPropProcLink

#include "dgdpdils.h"
#include "dgdpdvns.h"
#include "dgdpdvls.h"
#include "dgdpdpls.h"

#include "dgdpwdbu.h"
#include "dgdpwdbc.h"
#include "dgdpwro.h"
#include "dgdpwwu.h"
#include "dgdpwpr.h"
#include "dgdpwlo.h"
#include "dgdpwva.h"
#include "dgdpwbu.h"

#include "dgdpwfnp.h"   // CuDlgDomPropIceFacetAndPage
#include "dgdpwacd.h"   // CuDlgDomPropIceAccessDef
#include "dgdpwsbr.h"   // CuDlgDomPropIceSecRole
#include "dgdpwsbu.h"   // CuDlgDomPropIceSecDbuser

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgViewFrame::CuDlgViewFrame()
{
    m_nDlgID = 0;
}

CuDlgViewFrame::CuDlgViewFrame(UINT nDlgID)
{
    m_nDlgID = nDlgID;
}


CuDlgViewFrame::~CuDlgViewFrame()
{
}


BEGIN_MESSAGE_MAP(CuDlgViewFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CuDlgViewFrame)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
    ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnLeavingPage)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE,   OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgViewFrame message handlers

int CuDlgViewFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    return 0;
}

BOOL CuDlgViewFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
    CCreateContext context1;
    context1.m_pCurrentDoc   = new CuDlgViewDoc;

    switch (m_nDlgID)
    {
    //
    // ADDITIONAL CASES FOR DOM DETAIL WINDOWS
    //
    case IDD_DOMPROP_LOCATION:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropLocation);
         break;
    case IDD_DOMPROP_USER:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropUser);
         break;
    case IDD_DOMPROP_ROLE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropRole);
         break;
    case IDD_DOMPROP_PROF:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropProfile);
         break;
    case IDD_DOMPROP_INDEX:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIndex);
         break;
    case IDD_DOMPROP_PROC:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropProcedure);
         break;
    case IDD_DOMPROP_DB:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropDb);
         break;
    case IDD_DOMPROP_TABLE:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropTable);
        break;
    case IDD_DOMPROP_CONNECTION:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropConnection);
        break;
    case IDD_DOMPROP_DDB:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropDDb);
        break;
    case IDD_DOMPROP_STARPROC_L:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropProcLink);
         break;
    
    case IDD_DOMPROP_INDEX_L_SOURCE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIndexLinkSource);
         break;
    
    case IDD_DOMPROP_PROC_L_SOURCE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropProcLinkSource);
         break;
    
    case IDD_DOMPROP_VIEW_L_SOURCE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropViewLinkSource);
         break;
    
    case IDD_DOMPROP_VIEW_N_SOURCE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropViewNativeSource);
         break;
    
    // ICE branches
    case IDD_DOMPROP_ICE_DBCONNECTION:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceDbconnection);
         break;
    case IDD_DOMPROP_ICE_DBUSER:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceDbuser);
         break;
    case IDD_DOMPROP_ICE_PROFILE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceProfile);
         break;
    case IDD_DOMPROP_ICE_ROLE:
         context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceRole);
         break;
    case IDD_DOMPROP_ICE_WEBUSER:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceWebuser);
        break;
    case IDD_DOMPROP_ICE_SERVER_LOCATION:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceLocation);
        break;
    case IDD_DOMPROP_ICE_SERVER_VARIABLE:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceVariable);
        break;
    case IDD_DOMPROP_ICE_BUNIT:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceBunit);
        break;
    //
    // Ice DOM - continuation of.
    case IDD_DOMPROP_ICE_FACETNPAGE:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceFacetAndPage);
        break;
    case IDD_DOMPROP_ICE_BUNIT_ACCESSDEF:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceAccessDef);
        break;
    case IDD_DOMPROP_ICE_SEC_ROLE:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceSecRole);
        break;
    case IDD_DOMPROP_ICE_SEC_DBUSER:
        context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgDomPropIceSecDbuser);
        break;

    default:
        ASSERT (FALSE);
        break;
    }
    return CFrameWnd::OnCreateClient(lpcs, &context1);
}

LONG CuDlgViewFrame::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
    CView* pView = GetActiveView();
    if (pView)
        pView->SendMessage (WM_USER_IPMPAGE_UPDATEING, wParam, lParam);
    return 0L;
}

LONG CuDlgViewFrame::OnQueryType (WPARAM wParam, LPARAM lParam)
{
  CView* pView = GetActiveView();
  if (pView)
    return pView->SendMessage (WM_USER_DOMPAGE_QUERYTYPE, wParam, lParam);
  return 0L;
}

LONG CuDlgViewFrame::OnLeavingPage(WPARAM wParam, LPARAM lParam)
{
    CView* pView = GetActiveView();
    if (pView)
        pView->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, wParam, lParam);
    return 0L;
}


LONG CuDlgViewFrame::OnGetData (WPARAM wParam, LPARAM lParam)
{
    CView* pView = GetActiveView();
    if (pView)
        return (LRESULT)pView->SendMessage (WM_USER_IPMPAGE_GETDATA, wParam, lParam);
    return 0L;
}

LONG CuDlgViewFrame::OnLoad (WPARAM wParam, LPARAM lParam)
{
    CView* pView = GetActiveView();
    if (pView)
        pView->SendMessage (WM_USER_IPMPAGE_LOADING, wParam, lParam);
    return 0L;
}
