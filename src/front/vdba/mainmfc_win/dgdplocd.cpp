/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdplocd.cpp, Implementation file
**    Project  : INGRES II/ VDBA (DOM Right pane).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. Pie Chart.
**               For a given location, the partitions used by multiple databases.
**
** 
** Histoty: (after 26-Feb-2002)
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgdplocd.h"
#include "ipmprdta.h"
#include "dgdomc02.h"  // GETDIALOGDATAWITHDUP
#include "chartini.h"
#include "statfrm.h" 

extern "C" {
  #include "main.h"
  #include "dom.h"
  #include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuDlgDomPropLocationDbs::CuDlgDomPropLocationDbs(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropLocationDbs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropLocationDbs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pDlgFrame = NULL;
}


void CuDlgDomPropLocationDbs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropLocationDbs)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropLocationDbs, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropLocationDbs)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocationDbs message handlers

void CuDlgDomPropLocationDbs::PostNcDestroy() 
{
    delete this;
    CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropLocationDbs::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CRect r;
	CaPieChartCreationData crData;
	crData.m_bDBLocation = TRUE;
	crData.m_bUseLButtonDown = TRUE;
	crData.m_nLines = 2;
	crData.m_bShowPercentage = TRUE;
	crData.m_uIDS_LegendCount= IDS_DISKSPACE_NODATABASES;

	BOOL bPaleteAware  = FALSE;
	if (bPaleteAware)
	{
		//crData.m_pfnCreatePalette = &IPMCreatePalette;
	}

	try
	{
		GetClientRect (r);
		m_pDlgFrame = new CfStatisticFrame(CfStatisticFrame::nTypePie);
		m_pDlgFrame->Create(this, r, &crData);
		CaPieInfoData* pPieInfo = m_pDlgFrame->GetPieInformation();
		pPieInfo->Cleanup();
		pPieInfo->m_bDrawOrientation   = FALSE;
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

void CuDlgDomPropLocationDbs::OnSize(UINT nType, int cx, int cy) 
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


LONG CuDlgDomPropLocationDbs::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_SPECIAL;
}

LONG CuDlgDomPropLocationDbs::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  //
  // Get complimentary info on the object: areas usages
  //
  LOCATIONPARAMS locparams;
  memset (&locparams, 0, sizeof (locparams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  x_strcpy ((char *)locparams.objectname, (const char *)lpRecord->objName);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_LOCATION,
                               &locparams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);
    return 0L;
  }

  //
  // Allocate and fill big structure on location data necessary for graphic draw
  //
  LOCATIONDATAMIN locData;
  x_strcpy ((char *)locData.LocationName, (const char *)lpRecord->objName);
  x_strcpy ((char *)locData.LocationArea, (const char *)locparams.AreaName); // lpRecord->szComplim);
  locData.LocationUsages[0] = (locparams.bLocationUsage[LOCATIONDATABASE  ] ? ATTACHED_YES : ATTACHED_NO );   // DATA_PATH
  locData.LocationUsages[1] = (locparams.bLocationUsage[LOCATIONWORK      ] ? ATTACHED_YES : ATTACHED_NO );   // WORK_PATH
  locData.LocationUsages[2] = (locparams.bLocationUsage[LOCATIONJOURNAL   ] ? ATTACHED_YES : ATTACHED_NO );   // JNL_PATH
  locData.LocationUsages[3] = (locparams.bLocationUsage[LOCATIONCHECKPOINT] ? ATTACHED_YES : ATTACHED_NO );   // CHK_PATH
  locData.LocationUsages[4] = (locparams.bLocationUsage[LOCATIONDUMP      ] ? ATTACHED_YES : ATTACHED_NO );   // DMP_PATH

  // liberate detail structure
  FreeAttachedPointers (&locparams,  OT_LOCATION);

	try
	{
		CaPieInfoData* pPieInfo = m_pDlgFrame->GetPieInformation(); 
		pPieInfo->Cleanup();
		if (!Pie_Create(pPieInfo, nNodeHandle, (void*)&locData, TRUE))
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

LONG CuDlgDomPropLocationDbs::OnLoad (WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pClass = (LPCTSTR)wParam;
    CaPieInfoData* pData = (CaPieInfoData*)lParam;
    ASSERT (_tcscmp(pClass, _T("CuIpmPropertyDataDetailLocation")) == 0);
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

LONG CuDlgDomPropLocationDbs::OnGetData (WPARAM wParam, LPARAM lParam)
{
  CuIpmPropertyDataDetailLocation* pData = NULL;
  try
  {
    if (!m_pDlgFrame)
        return (LRESULT)pData;
    pData = new CuIpmPropertyDataDetailLocation ();
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

void CuDlgDomPropLocationDbs::ResetDisplay()
{

}
