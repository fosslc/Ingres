/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgILSumm.cpp, Implementation file.                                    //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail (Summary) page of Locations.            //              
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "ipmprdta.h"
#include "dgilsumm.h"
#include "statlege.h"
#include "statdoc.h"
#include "vtree.h"
#include "chartini.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgIpmSummaryLocations::CuDlgIpmSummaryLocations(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmSummaryLocations::IDD, pParent)
{
	m_pDlgFrame = NULL;
	//{{AFX_DATA_INIT(CuDlgIpmSummaryLocations)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmSummaryLocations::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmSummaryLocations)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmSummaryLocations, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmSummaryLocations)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_APPLYNOW, OnApply)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmSummaryLocations message handlers

void CuDlgIpmSummaryLocations::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmSummaryLocations::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CDialog::OnInitDialog();
	CStatic* s = (CStatic*)GetDlgItem (IDC_MFC_STATIC1);
	CRect r;
	VERIFY (m_cListLegend.SubclassDlgItem (IDC_MFC_LIST1, this));
	s->GetWindowRect (r);
	ScreenToClient (r);
	m_pDlgFrame = new CuStatisticBarFrame(CuStatisticBarFrame::nTypeBar);
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

void CuDlgIpmSummaryLocations::OnSize(UINT nType, int cx, int cy) 
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
	GetClientRect (rDlg);
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

LONG CuDlgIpmSummaryLocations::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
		CuStatisticBarDoc* pDoc   =  (CuStatisticBarDoc*)m_pDlgFrame->GetDoc();
		CuDiskInfoData* pDiskInfo = pDoc->m_pDiskInfo;
		if (Chart_Create(pDiskInfo, nNodeHandle))
		{
			m_pDlgFrame->DrawLegend (&m_cListLegend, TRUE);
		}
		else
		{
			//CString strMsg = _T("Disk Space Statistic error");
			BfxMessageBox (VDBA_MfcResourceString(IDS_E_DISK_SPACE), MB_ICONEXCLAMATION|MB_OK);
			pDiskInfo->Cleanup();
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

LONG CuDlgIpmSummaryLocations::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	CuIpmPropertyDataSummaryLocations* pData = (CuIpmPropertyDataSummaryLocations*)lParam;
	ASSERT (lstrcmp (pClass, "CuIpmPropertyDataSummaryLocations") == 0);
	if (!pData)
		return 0L;
	try
	{
		ResetDisplay();
		CuStatisticBarDoc* pDoc =  (CuStatisticBarDoc*)m_pDlgFrame->GetDoc();
		pDoc->m_nBarWidth = pData->m_nBarWidth;
		pDoc->m_nUnit  = pData->m_nUnit;
		pDoc->SetDiskInfo (pData->m_pDiskInfo);
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

LONG CuDlgIpmSummaryLocations::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataSummaryLocations* pData = NULL;
	try
	{
		CuStatisticBarDoc* pDoc =  (CuStatisticBarDoc*)m_pDlgFrame->GetDoc();
		pData = new CuIpmPropertyDataSummaryLocations ();
		pData->m_nBarWidth = pDoc->m_nBarWidth;
		pData->m_nUnit  = pDoc->m_nUnit;
		pData->m_pDiskInfo = pDoc->m_pDiskInfo;
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}


LONG CuDlgIpmSummaryLocations::OnApply (WPARAM wParam, LPARAM lParam)
{
	return 0L;
}

void CuDlgIpmSummaryLocations::ResetDisplay()
{

}
