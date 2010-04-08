/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdplocs.cpp, Implementation file
**    Project  : INGRES II/ VDBA (DOM Right pane).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control.
**               For a given location, the pie shows the amount of the disk space
**               occupied by that location in a specific disk unit.
**
** 
** Histoty: (after 08-Apr-2003)
** 09-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdplocs.h"
#include "ipmprdta.h"
#include "dgdomc02.h"  // GETDIALOGDATAWITHDUP
#include "chartini.h"

extern "C" {
  #include "main.h"
  #include "dom.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocationSpace dialog


CuDlgDomPropLocationSpace::CuDlgDomPropLocationSpace(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropLocationSpace::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropLocationSpace)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pDlgFrame = NULL;
}


void CuDlgDomPropLocationSpace::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropLocationSpace)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropLocationSpace, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropLocationSpace)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocationSpace message handlers

void CuDlgDomPropLocationSpace::PostNcDestroy() 
{
    delete this;
    CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropLocationSpace::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CaPieChartCreationData crData;
	crData.m_bDrawAxis = FALSE;
	crData.m_bDBLocation = FALSE;
	crData.m_bUseLButtonDown = TRUE;
	crData.m_nLines = 2;
	crData.m_bShowPercentage = TRUE;
//	if (bPaleteAware)
//	{
//		crData.m_pfnCreatePalette = &IPMCreatePalette;
//	}

	try
	{
		CRect r;
		GetClientRect (r);
		m_pDlgFrame = new CfStatisticFrame(CfStatisticFrame::nTypePie);
		m_pDlgFrame->Create(this, r, &crData);
		CaPieInfoData* pPieInfo = m_pDlgFrame->GetPieInformation();
		pPieInfo->Cleanup();
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

void CuDlgDomPropLocationSpace::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgDomPropLocationSpace::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!m_pDlgFrame)
		return;
	if (!IsWindow(m_pDlgFrame->m_hWnd))
		return;
	CRect rDlg;
	GetClientRect(rDlg);
	m_pDlgFrame->MoveWindow (rDlg);
}


LONG CuDlgDomPropLocationSpace::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_SPECIAL;
}

LONG CuDlgDomPropLocationSpace::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  LOCATIONDATAMIN locData;
  x_strcpy ((char *)locData.LocationName, (const char *)lpRecord->objName);
  x_strcpy ((char *)locData.LocationArea, (const char *)lpRecord->szComplim);
  // Just in case - Normally not used by Create method with 2 parameters
  locData.LocationUsages[0] = ATTACHED_NO;   // DATA_PATH
  locData.LocationUsages[1] = ATTACHED_NO;   // WORK_PATH
  locData.LocationUsages[2] = ATTACHED_NO;   // JNL_PATH
  locData.LocationUsages[3] = ATTACHED_NO;   // CHK_PATH
  locData.LocationUsages[4] = ATTACHED_NO;   // DMP_PATH

  try
  {
    ResetDisplay();
    CaPieInfoData* pPieInfo = m_pDlgFrame->GetPieInformation();
    pPieInfo->Cleanup();
    if (!Pie_Create(pPieInfo, nNodeHandle, &locData))
    {
        AfxMessageBox (IDS_E_LOCATION_SPACE, MB_ICONEXCLAMATION|MB_OK);
        pPieInfo->Cleanup();
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

LONG CuDlgDomPropLocationSpace::OnLoad (WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pClass = (LPCTSTR)wParam;
    CaPieInfoData* pData = (CaPieInfoData*)lParam;
    ASSERT (_tcscmp (pClass, _T("CuIpmPropertyDataPageLocation")) == 0);
    ResetDisplay();
    if (!pData)
        return 0L;
    try
    {
       if (!m_pDlgFrame)
           return 0;
        m_pDlgFrame->SetPieInformation(pData);
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

LONG CuDlgDomPropLocationSpace::OnGetData (WPARAM wParam, LPARAM lParam)
{
    CuIpmPropertyDataPageLocation* pData = NULL;
    try
    {
        pData = new CuIpmPropertyDataPageLocation ();
        if (lParam == GETDIALOGDATAWITHDUP) {
          // Duplicate object
            pData->m_pPieInfo = new CaPieInfoData(m_pDlgFrame->GetPieInformation());
        }
        else
            pData->m_pPieInfo = m_pDlgFrame->GetPieInformation(); // no duplication
    }
    catch (CMemoryException* e)
    {
        VDBA_OutOfMemoryMessage();
        e->Delete();
    }
    return (LRESULT)pData;
}

void CuDlgDomPropLocationSpace::ResetDisplay()
{

}