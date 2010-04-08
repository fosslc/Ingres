/********************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres II/ VDBA.
**
**   09-May-2001 (schph01)
**     SIR 102509 display the informations on raw data location.
**   06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
**   13-Dec-2001 (noifr01)
**     restored the trailing } at the end of the file 
********************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdploc.h"
#include "domseri.h"    // serialization class

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgDomPropLocation, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocation dialog


CuDlgDomPropLocation::CuDlgDomPropLocation()
	: CFormView(CuDlgDomPropLocation::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropLocation)
	//}}AFX_DATA_INIT
  m_refreshParams.InitializeRefreshParamsType(OT_LOCATION);
}


void CuDlgDomPropLocation::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropLocation)
	DDX_Control(pDX, IDC_STATIC_CHECKAREA, m_ctrlStaticCheckArea);
	DDX_Control(pDX, IDC_STATIC_PCT_RAW, m_ctrlStaticPctRaw);
	DDX_Control(pDX, IDC_EDIT_PCT_RAW2, m_ctrlEditPctRaw);
	DDX_Control(pDX, IDC_CHECKRAWAREA, m_chkRawArea);
	DDX_Control(pDX, IDC_AREA, m_editArea);
	DDX_Control(pDX, IDC_CHECK_WORK, m_chkWork);
	DDX_Control(pDX, IDC_CHECK_JOURNAL, m_chkJournal);
	DDX_Control(pDX, IDC_CHECK_DUMP, m_chkDump);
	DDX_Control(pDX, IDC_CHECK_DATABASE, m_chkDatabase);
	DDX_Control(pDX, IDC_CHECK_CHECKPOINT, m_chkCheckpoint);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropLocation, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropLocation)
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocation diagnostics

#ifdef _DEBUG
void CuDlgDomPropLocation::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropLocation::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropLocation message handlers

LONG CuDlgDomPropLocation::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropLocation::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      break;

    case FILTER_DOM_BKREFRESH_DETAIL:
      if (m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
        break;    // need to update
      else
        return 0; // no need to update
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the object
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
    m_Area        = "";
    m_bDatabase   = FALSE;
    m_bWork       = FALSE;
    m_bJournal    = FALSE;
    m_bCheckPoint = FALSE;
    m_bDump       = FALSE;
    // leave m_refreshParams as is
    return 0L;
  }

  // Update refresh info
  m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Area        = locparams.AreaName;
  m_bDatabase   = locparams.bLocationUsage[LOCATIONDATABASE  ];
  m_bWork       = locparams.bLocationUsage[LOCATIONWORK      ];
  m_bJournal    = locparams.bLocationUsage[LOCATIONJOURNAL   ];
  m_bCheckPoint = locparams.bLocationUsage[LOCATIONCHECKPOINT];
  m_bDump       = locparams.bLocationUsage[LOCATIONDUMP      ];
  m_iRawPct     = locparams.iRawPct;

  // liberate detail structure
  FreeAttachedPointers (&locparams,  OT_LOCATION);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropLocation::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataLocation") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataLocation* pData = (CuDomPropDataLocation*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataLocation)));

  // extract info from received class and parse it to dialog class members
  m_Area        = pData->m_Area       ;
  m_bDatabase   = pData->m_bDatabase  ;
  m_bWork       = pData->m_bWork      ;
  m_bJournal    = pData->m_bJournal   ;
  m_bCheckPoint = pData->m_bCheckPoint;
  m_bDump       = pData->m_bDump      ;
  m_refreshParams = pData->m_refreshParams;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropLocation::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW
  CuDomPropDataLocation* pData = new CuDomPropDataLocation;

  // Fill
  pData->m_Area        = m_Area       ;
  pData->m_bDatabase   = m_bDatabase  ;
  pData->m_bWork       = m_bWork      ;
  pData->m_bJournal    = m_bJournal   ;
  pData->m_bCheckPoint = m_bCheckPoint;
  pData->m_bDump       = m_bDump      ;
  pData->m_iRawPct     = m_iRawPct    ;
  pData->m_refreshParams = m_refreshParams;

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropLocation::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables for display refresh
  m_editArea.SetWindowText(m_Area);
  m_chkDatabase.SetCheck  (m_bDatabase  ? 1: 0);
  m_chkWork.SetCheck      (m_bWork      ? 1: 0);
  m_chkJournal.SetCheck   (m_bJournal   ? 1: 0);
  m_chkCheckpoint.SetCheck(m_bCheckPoint? 1: 0);
  m_chkDump.SetCheck      (m_bDump      ? 1: 0);

  if (GetOIVers() < OIVERS_26)
  {
    m_ctrlStaticCheckArea.ShowWindow(SW_HIDE);
    m_chkRawArea.ShowWindow(SW_HIDE);
  }
  else
  {
    m_ctrlStaticCheckArea.ShowWindow(SW_SHOW);
    m_chkRawArea.ShowWindow(SW_SHOW);
  }

  if (m_iRawPct>0)
  {
    // show all controls on "Raw Blocks" informations
    // Checked/unckecked "raw blocks" information.
    m_chkRawArea.SetCheck   (TRUE);
    CString csTemp;
    csTemp.Format("%d",m_iRawPct);
    m_ctrlEditPctRaw.ShowWindow(SW_SHOW);
    m_ctrlEditPctRaw.SetWindowText(csTemp);
    m_ctrlStaticPctRaw.ShowWindow(SW_SHOW);
    m_ctrlStaticPctRaw.EnableWindow(TRUE);
  }
  else
  {
    m_chkRawArea.SetCheck (FALSE);
    // hide all controls on "Raw Blocks" informations
    m_ctrlEditPctRaw.SetWindowText(_T(""));
    m_ctrlEditPctRaw.ShowWindow(SW_HIDE);
    m_ctrlStaticPctRaw.ShowWindow(SW_HIDE);
  }
}

void CuDlgDomPropLocation::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_editArea.SetWindowText(_T(""));
  m_chkDatabase.SetCheck  (0);
  m_chkWork.SetCheck      (0);
  m_chkJournal.SetCheck   (0);
  m_chkCheckpoint.SetCheck(0);
  m_chkDump.SetCheck      (0);
  m_chkRawArea.SetCheck   (0);

  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
