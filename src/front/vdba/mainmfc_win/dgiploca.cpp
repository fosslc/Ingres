/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIpLoca.cpp, Implementation file.                                    //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control.                                                //              
//               The Location page. (For drawing the pie chart:                        //
//               (loc X, Other location, Other, Free)                                  //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "dgiploca.h"
#include "ipmprdta.h"
#include "piedoc.h"
#include "vtree.h"
#include "chartini.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLocation dialog


CuDlgIpmPageLocation::CuDlgIpmPageLocation(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLocation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLocation)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageLocation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLocation)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLocation, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLocation)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLocation message handlers

void CuDlgIpmPageLocation::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageLocation::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CStatic* s = (CStatic*)GetDlgItem (IDC_MFC_STATIC1);
	CRect	 r;
	VERIFY (m_cListLegend.SubclassDlgItem (IDC_MFC_LIST1, this));
	s->GetWindowRect (r);
	ScreenToClient (r);
	m_pDlgFrame = new CuStatisticBarFrame(CuStatisticBarFrame::nTypePie);
	m_pDlgFrame->Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this);
	m_pDlgFrame->ShowWindow (SW_SHOW);
	m_pDlgFrame->InitialUpdateFrame (NULL, TRUE);
	m_pDlgFrame->DrawLegend (&m_cListLegend, TRUE);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLocation::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgIpmPageLocation::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CStatic* s = (CStatic*)GetDlgItem (IDC_MFC_STATIC1);
	if (!s)
		return;
	if (!IsWindow (s->m_hWnd))
		return;
	if (!IsWindow (m_cListLegend.m_hWnd))
		return;
	CRect r, rDlg, r2;
	int H;

	GetClientRect(rDlg);
	m_cListLegend.GetWindowRect (r);
	ScreenToClient (r);
	H = r.Height();
	r.bottom = rDlg.bottom;
	r.top = rDlg.bottom - H;
	r.left = rDlg.left;
	r.right  = rDlg.right; 
	m_cListLegend.MoveWindow (r);
	rDlg.bottom = r.top - 4;
	m_pDlgFrame->MoveWindow (rDlg);
}


LONG CuDlgIpmPageLocation::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int ires = RES_ERR, nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_IPM_EXPRESS_REFRESH:
		break;
	default:
		return 0L;
	}
	try
	{
		ResetDisplay();
		CuStatisticPieDoc* pDoc   =  (CuStatisticPieDoc*)m_pDlgFrame->GetDoc();
		CuPieInfoData* pPieInfo = pDoc->GetPieInfo();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_locStruct, 0, sizeof (m_locStruct));
			m_locStruct = *(LPLOCATIONDATAMIN)pUps->pStruct;
		}

		if (Pie_Create(pPieInfo, nNodeHandle, (LPLOCATIONDATAMIN)&m_locStruct))
		{
			m_pDlgFrame->DrawLegend (&m_cListLegend, TRUE);
		}
		else
		{
			CString strMsg;
			strMsg.LoadString(IDS_E_LOCATION_SPACE);//= _T("Location Space Statistic error");
			BfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
			pPieInfo->Cleanup();
			m_cListLegend.ResetContent();
		}
		pDoc->UpdateAllViews (NULL);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLocation::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	CuPieInfoData* pData = (CuPieInfoData*)lParam;
	ASSERT (lstrcmp (pClass, "CuIpmPropertyDataPageLocation") == 0);
	if (!pData)
		return 0L;
	try
	{
		ResetDisplay();
		CuStatisticPieDoc* pDoc =  (CuStatisticPieDoc*)m_pDlgFrame->GetDoc();
		pDoc->SetPieInfo (pData);
		pDoc->UpdateAllViews (NULL);
		m_pDlgFrame->DrawLegend (&m_cListLegend, TRUE);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLocation::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLocation* pData = NULL;
	try
	{
		CuStatisticPieDoc* pDoc =  (CuStatisticPieDoc*)m_pDlgFrame->GetDoc();
		pData = new CuIpmPropertyDataPageLocation ();
		pData->m_pPieInfo = pDoc->GetPieInfo();
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageLocation::ResetDisplay()
{

}