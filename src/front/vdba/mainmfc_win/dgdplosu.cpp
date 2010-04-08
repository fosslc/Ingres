/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdplocu.cpp, Implementation file
**    Project  : INGRES II/ VDBA (DOM Right pane).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control (locations and disks units represented by bars). 
**
** 
** Histoty: (after 08-Apr-2003)
** 09-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmprdta.h"
#include "dgdplosu.h"
#include "dgdomc02.h"  // GETDIALOGDATAWITHDUP
#include "chartini.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgDomPropLocations::CuDlgDomPropLocations(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropLocations::IDD, pParent)
{
    m_pDlgFrame = NULL;
    //{{AFX_DATA_INIT(CuDlgDomPropLocations)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropLocations::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CuDlgDomPropLocations)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropLocations, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropLocations)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
    ON_MESSAGE (WM_USER_APPLYNOW, OnApply)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocations message handlers

void CuDlgDomPropLocations::PostNcDestroy() 
{
    delete this;    
    CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropLocations::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CaPieChartCreationData crData;
	crData.m_bUseLButtonDown = TRUE;
	crData.m_nLines = 2;
	crData.m_bShowPercentage = TRUE;
	try
	{
		CRect r;
		GetClientRect(r);
		m_pDlgFrame = new CfStatisticFrame(CfStatisticFrame::nTypeBar);
		m_pDlgFrame->Create(this, r, &crData);
		CaBarInfoData* pBarInfo = m_pDlgFrame->GetBarInformation();
		pBarInfo->Cleanup();
	}
	catch (CMemoryException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (...)
	{

	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropLocations::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!m_pDlgFrame)
		return;
	if (!IsWindow(m_pDlgFrame->m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);
	m_pDlgFrame->MoveWindow (rDlg);
}

LONG CuDlgDomPropLocations::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_SPECIAL;
}

LONG CuDlgDomPropLocations::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  // cast received parameters
  int nNodeHandle = (int)wParam;
  LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
  ASSERT (nNodeHandle != -1);
  ASSERT (pUps);

  // ignore selected actions on filters
  switch (pUps->nIpmHint)
  {
    case 0:
    //case FILTER_DOM_SYSTEMOBJECTS:
    //case FILTER_DOM_BASEOWNER:
    //case FILTER_DOM_OTHEROWNER:
    //case FILTER_DOM_BKREFRESH:          // Special item - no refresh code
    //case FILTER_DOM_BKREFRESH_DETAIL:   // Special item - no refresh code
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  try
  {
      ResetDisplay();
      CaBarInfoData* pDiskInfo = m_pDlgFrame->GetBarInformation();
      pDiskInfo->Cleanup();
      if (!Chart_Create(pDiskInfo, nNodeHandle))
      {
          AfxMessageBox (IDS_E_DISK_SPACE, MB_ICONEXCLAMATION|MB_OK);
          pDiskInfo->Cleanup();
      }
      m_pDlgFrame->UpdatePieChart();
      m_pDlgFrame->UpdateLegend();
  }
  catch (CMemoryException* e)
  {
      VDBA_OutOfMemoryMessage();
      e->Delete();
  }
  return 0L;
}

LONG CuDlgDomPropLocations::OnLoad (WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pClass = (LPCTSTR)wParam;
    CuIpmPropertyDataSummaryLocations* pData = (CuIpmPropertyDataSummaryLocations*)lParam;
    ASSERT (_tcscmp (pClass, _T("CuIpmPropertyDataSummaryLocations")) == 0);
    ResetDisplay();
    if (!pData)
        return 0L;
    try
    {
        //pDoc->m_nBarWidth = pData->m_nBarWidth;
        //pDoc->m_nUnit     = pData->m_nUnit;
        //pDoc->SetDiskInfo (pData->m_pDiskInfo);
        m_pDlgFrame->SetBarInformation(pData->m_pDiskInfo);
        m_pDlgFrame->UpdatePieChart();
        m_pDlgFrame->UpdateLegend();
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }
    return 0L;
}

LONG CuDlgDomPropLocations::OnGetData (WPARAM wParam, LPARAM lParam)
{
    CuIpmPropertyDataSummaryLocations* pData = NULL;
    try
    {
        pData = new CuIpmPropertyDataSummaryLocations ();
        //pData->m_nBarWidth = pDoc->m_nBarWidth;
        //pData->m_nUnit     = pDoc->m_nUnit;
        if (lParam == GETDIALOGDATAWITHDUP) {
          // Duplicate object
          pData->m_pDiskInfo = new CaBarInfoData(m_pDlgFrame->GetBarInformation());
        }
        else
          pData->m_pDiskInfo = m_pDlgFrame->GetBarInformation();
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }
    return (LRESULT)pData;
}


LONG CuDlgDomPropLocations::OnApply (WPARAM wParam, LPARAM lParam)
{
    return 0L;
}

void CuDlgDomPropLocations::ResetDisplay()
{

}
