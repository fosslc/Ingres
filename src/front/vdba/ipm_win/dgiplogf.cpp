/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiplogf.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Log File page. (For drawing the pie chart)
**
** History:
**
** xx-Feb-1998 (uk$so01)
**    Created
** 18-Mar-2009 (drivi01)
**    Add parentheses around a statement to clarify the precedence.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgiplogf.h"
#include "ipmprdta.h"
extern BOOL CreateMultiUsePalette(CPalette* pMultiUsePalette); // defined in libwctrl.lib

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Method specific for Logfile Diagram:
// The reserve starts from the InUse position in the clockwise direction:
static BOOL Pie_Create(CaPieInfoData* pPieInfo, CdIpmDoc* pIpmDoc, LOGHEADERDATAMIN* pData)
{
	COLORREF tab_colour [] = 
	{
		RGB (192,  0,  0),  // Red
		RGB (  0,192,  0),  // Green
		RGB (  0,  0,192),  // Bleu
		RGB (255,255,  0),  // Yellow
		RGB (255,  0,255),  // Pink
		RGB (  0,255,255),  // Blue light
		RGB (  0,128,  0),  // Green dark
		RGB (128,  0,128),  // violet
		RGB (  0,128,128),  // Blue
		RGB (128,128,  0),  // Brun (Marron)
		RGB (128,128,128),  // Gris Sombre
		RGB (255,  0,  0),  // Red
		RGB (  0,255,  0),  // Green
		RGB (192,  0,255),  // Violet
		RGB (  0,192,192),  // Blue
		RGB (  0,  0,192),  // Blue
		RGB (  0,  0,128),  // Blue
		RGB (192,192,  0),  // orange
		RGB (255,  0,128),  // Pink dark
		RGB (192,220,192),  // Green 
		RGB ( 64,  0,128),  // Violet 
		RGB (128, 64,  0),  // Brown 
		RGB (128,128, 64),  // Green 
		RGB (  0,  0,  0)   // Black
	};
	CString strUnit;      // _T("Block(s)")
	CString strDiagram;   // _T("Log File Diagram (Total = %d Blocks)")
	CString strInUse;     // _T("In Use");
	CString strReserved;  // _T("Reserved");
	CString strAvailable; // _T("Available");
	COLORREF crAvailable = tab_colour [1]; // Green
	COLORREF crReserve   = tab_colour [3]; // Yellow
	COLORREF crInUse     = tab_colour [2]; // Blue
	double dPercent      = 0.0;
	double dRest         = 100.0;
	double dDegStart     = 90.0;           // Start at 90 (default if no reserve)
	strUnit.LoadString(IDS_BLOCKS);
	strDiagram.LoadString(IDS_LOGFILE_DIAGRAM);
	strInUse.LoadString(IDS_INUSE);
	strReserved.LoadString(IDS_RESERVED);
	strAvailable.LoadString(IDS_AVAILABLE);

	pPieInfo->m_strTitle.Format (strDiagram, pData->block_count);
	pPieInfo->m_strFormat          = _T("%s = (%0.2f%%, %d %s)");
	pPieInfo->m_bDrawOrientation   = TRUE;

	pPieInfo->Cleanup();
	pPieInfo->SetCapacity ((double)pData->block_count, strUnit);

	//
	// Calculate the Start position of the InUse:
	if (pData->begin_block > 0)
	{
		dDegStart+= (double)pData->begin_block*100.0*3.6/(double)pData->block_count;
	}

	//
	// Calculate the Position and Percentage of Reserve:
	if (pData->reserved_blocks > 0)
	{
		double degStartReserve = 0.0;
		dPercent = (double)pData->reserved_blocks * 100.0 / (double)pData->block_count;
		degStartReserve = dDegStart - dPercent*3.6;
		if (degStartReserve < 0.0)
			degStartReserve = 360.0 + degStartReserve;
		dDegStart = degStartReserve;
		pPieInfo->AddPie (strReserved, dPercent, crReserve, dDegStart); 
		dRest -= dPercent;
	}
	else
		pPieInfo->AddLegend (strReserved, crReserve);

	if (pData->inuse_blocks > 0)
	{
		//
		// Blue.
		dPercent = (double)pData->inuse_blocks * 100.0 / (double)pData->block_count;
		pPieInfo->AddPie (strInUse, dPercent, crInUse, dDegStart); 
		dRest -= dPercent;
	}
	else
		pPieInfo->AddLegend (strInUse, crInUse);

	if (dRest > 0)
		pPieInfo->AddPie (strAvailable, dRest, crAvailable, dDegStart); 
	else
		pPieInfo->AddLegend (strAvailable, crAvailable);

	pPieInfo->m_nBlockNextCP= pData->next_cp_block;
	pPieInfo->m_nBlockPrevCP= pData->cp_block;
	pPieInfo->m_nBlockBOF   = pData->begin_block;
	pPieInfo->m_nBlockEOF   = pData->end_block;
	pPieInfo->m_nBlockTotal = pData->block_count;

	return TRUE;
}

CuDlgIpmPageLogFile::CuDlgIpmPageLogFile(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLogFile::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLogFile)
	m_strStatus = _T("");
	//}}AFX_DATA_INIT
	m_pPieCtrl = NULL;
}


void CuDlgIpmPageLogFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLogFile)
	DDX_Control(pDX, IDC_EDIT5, m_cEditStatus);
	DDX_Control(pDX, IDC_STATIC6, m_cStaticStatus);
	DDX_Text(pDX, IDC_EDIT5, m_strStatus);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLogFile, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLogFile)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLogFile message handlers

void CuDlgIpmPageLogFile::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CALLBACK IPMCreatePalette(CPalette* pDetailedPalette)
{
	return CreateMultiUsePalette(pDetailedPalette);
}

BOOL CuDlgIpmPageLogFile::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CStatic* s = (CStatic*)GetDlgItem (IDC_STATIC1);
	CRect    r;
	s->GetWindowRect (r);
	ScreenToClient (r);

	BOOL bPaleteAware = FALSE;
	// Determine whether we need a palette, and use it if found so
	CPalette* pOldPalette = NULL;
	CPalette draw3dPalette;
	CClientDC dc(this);
	if ((dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) != 0)
	{
		int palSize  = dc.GetDeviceCaps(SIZEPALETTE);
		if (palSize > 0) 
		{
			ASSERT (palSize >= 256);
			if (palSize >= 256)
				bPaleteAware = TRUE;
		}
	}

	CaPieChartCreationData crData;
	crData.m_bDBLocation = FALSE;
	crData.m_bUseLButtonDown = TRUE;
	crData.m_nLines = 2;
	crData.m_bShowPercentage = TRUE;
	crData.m_uIDS_IncreasePaneSize = IDS_PLEASE_ENLARGEPANE;
	if (bPaleteAware)
	{
		crData.m_pfnCreatePalette = &IPMCreatePalette;
	}

	try
	{
		m_pPieCtrl = new CfStatisticFrame(CfStatisticFrame::nTypePie);
		m_pPieCtrl->Create(this, r, &crData);
		CaPieInfoData* pPieInfo = m_pPieCtrl->GetPieInformation();
		pPieInfo->Cleanup();
		pPieInfo->m_bDrawOrientation   = TRUE;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{

	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLogFile::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgIpmPageLogFile::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CStatic* s = (CStatic*)GetDlgItem (IDC_STATIC1);
	if (!s)
		return;
	if (!IsWindow (s->m_hWnd))
		return;
	if (!m_pPieCtrl)
		return;
	if (!IsWindow (m_pPieCtrl->m_hWnd))
		return;
	CRect r, rDlg, r2;
	GetClientRect(rDlg);
	//
	// First move all controls, except the static for pie chart, to the bottom:

	//
	// 1) Status:
	int nHeight;
	m_cStaticStatus.GetWindowRect (r);
	ScreenToClient (r);
	nHeight  = r.Height();
	r.bottom = rDlg.bottom -2;
	r.top    = r.bottom - nHeight;
	m_cStaticStatus.MoveWindow (r);

	m_cEditStatus.GetWindowRect (r);
	ScreenToClient (r);
	nHeight  = r.Height();
	r.bottom = rDlg.bottom -2;
	r.top    = r.bottom - nHeight;
	m_cEditStatus.MoveWindow (r);

	rDlg.bottom = r.top - 2;
	m_pPieCtrl->MoveWindow (rDlg);
}


LONG CuDlgIpmPageLogFile::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CaPieInfoData* pPieInfo = NULL;
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
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		memset (&m_headStruct, 0, sizeof (m_headStruct));

		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOGHEADER);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_headStruct);
#if defined (_SIMULATED_DATA)
		ires = RES_SUCCESS;
		m_headStruct.block_count = 360;
		m_headStruct.begin_block = 300;
		m_headStruct.end_block   = 390;
		m_headStruct.reserved_blocks=45;
		m_headStruct.inuse_blocks=90;
		m_headStruct.next_cp_block=130;
		m_headStruct.cp_block=280;
#endif

		if (bOK)
		{
			pPieInfo = m_pPieCtrl->GetPieInformation();
			if (Pie_Create(pPieInfo, pDoc, &m_headStruct))
			{
				m_pPieCtrl->UpdatePieChart();
				m_pPieCtrl->UpdateLegend();
			}
			m_strStatus         = m_headStruct.status;
			UpdateData (FALSE);
		}
		else
		{
			CString csNotAvailable;
			csNotAvailable.LoadString (IDS_NOT_AVAILABLE);
			m_strStatus = csNotAvailable;//_T("n/a")
			UpdateData (FALSE);
			pPieInfo = m_pPieCtrl->GetPieInformation();
			if (pPieInfo)
				pPieInfo->Cleanup();
			pPieInfo->m_bError = TRUE;
			AfxMessageBox (IDS_E_LOG_FILE_ERROR, MB_ICONEXCLAMATION|MB_OK);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		CString csNotAvailable;
		csNotAvailable.LoadString (IDS_NOT_AVAILABLE);
		m_strStatus = csNotAvailable;//_T("n/a")
		UpdateData (FALSE);
		pPieInfo = m_pPieCtrl->GetPieInformation();
		if (pPieInfo)
			pPieInfo->Cleanup();
		pPieInfo->m_bError = TRUE;
		AfxMessageBox (IDS_E_LOG_FILE_ERROR, MB_ICONEXCLAMATION|MB_OK);
	}

	return 0L;
}

LONG CuDlgIpmPageLogFile::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageLogFile")) == 0);
	CuIpmPropertyDataPageLogFile* pData = (CuIpmPropertyDataPageLogFile*)lParam;
	CaPieInfoData* pPie = pData->m_pPieInfo;
	if (!pData)
		return 0L;
	try
	{
		ResetDisplay();
		m_pPieCtrl->SetPieInformation (pPie);
		m_pPieCtrl->UpdatePieChart();
		m_pPieCtrl->UpdateLegend();
		m_strStatus = pData->m_strStatus;
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLogFile::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLogFile* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataPageLogFile ();
		pData->m_strStatus = m_strStatus;
		pData->m_pPieInfo = m_pPieCtrl->GetPieInformation();
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageLogFile::ResetDisplay()
{

}
