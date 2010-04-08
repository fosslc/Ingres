/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DgDPIdx.cpp : implementation file
**    Project  : Ingres VisualDBA
**
**  History:
**
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  02-feb-2004 (somsa01)
**    Updated to "typecast" UCHAR into CString before concatenation.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpidx.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "dbaginfo.h"   // GetGWType
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgDomPropIndex, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIndex dialog


CuDlgDomPropIndex::CuDlgDomPropIndex()
	: CFormView(CuDlgDomPropIndex::IDD)
{
  m_bMustInitialize = TRUE;

	//{{AFX_DATA_INIT(CuDlgDomPropIndex)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_INDEX);
}

CuDlgDomPropIndex::~CuDlgDomPropIndex()
{
}

void CuDlgDomPropIndex::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIndex)
	DDX_Control(pDX, IDC_STATIC_PERSIST, m_stPersist);
	DDX_Control(pDX, IDC_LIST_LOCATIONS, m_lbLocations);
	DDX_Control(pDX, IDC_LIST_COLUMNS, m_clistColumns);
	DDX_Control(pDX, IDC_EDIT_PGSIZE, m_edPgSize);
	DDX_Control(pDX, IDC_EDIT_UNIQUE, m_edUnique);
	DDX_Control(pDX, IDC_EDIT_TBLNAME, m_edParent);
	DDX_Control(pDX, IDC_EDIT_IDXSCHEMA, m_edIdxSchema);
	DDX_Control(pDX, IDC_EDIT_FILLFACTOR, m_edFillFact);
	DDX_Control(pDX, IDC_EDIT_EXTEND, m_edExtend);
	DDX_Control(pDX, IDC_EDIT_ALLOCATEDPAGES, m_edAllocatedPages);
	DDX_Control(pDX, IDC_EDIT_ALLOCATION, m_edAlloc);
	DDX_Control(pDX, IDC_STATIC_STRUCT4, m_stStruct4);
	DDX_Control(pDX, IDC_STATIC_STRUCT3, m_stStruct3);
	DDX_Control(pDX, IDC_STATIC_STRUCT2, m_stStruct2);
	DDX_Control(pDX, IDC_STATIC_COMPRESSION, m_stCompress);
	DDX_Control(pDX, IDC_STATIC_STRUCT1, m_stStruct1);
	DDX_Control(pDX, IDC_EDIT_STRUCT4, m_edStruct4);
	DDX_Control(pDX, IDC_EDIT_STRUCT3, m_edStruct3);
	DDX_Control(pDX, IDC_EDIT_STRUCT2, m_edStruct2);
	DDX_Control(pDX, IDC_EDIT_STRUCT1, m_edStruct1);
	DDX_Control(pDX, IDC_EDIT_STRUCT, m_edStruct);
	DDX_Control(pDX, IDC_CHECK_PERSISTENCE, m_chkPersist);
	DDX_Control(pDX, IDC_CHECK_COMPRESSION, m_chkCompress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIndex, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIndex)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIndex diagnostics

#ifdef _DEBUG
void CuDlgDomPropIndex::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIndex::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIndex message handlers

void CuDlgDomPropIndex::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIndex::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIndex::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      if (m_Data.m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
        break;    // need to update
      else
        return 0; // no need to update
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the object
  INDEXPARAMS indexparams;
  memset (&indexparams, 0, sizeof (indexparams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  x_strcpy ((char *)indexparams.DBName,     (const char *)lpRecord->extra);    // parent 0
  x_strcpy ((char *)indexparams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)indexparams.objectowner, (const char *)lpRecord->ownerName);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_INDEX,
                               &indexparams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIndex tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    if (GetOIVers() == OIVERS_NOTOI)
      m_Data.m_bEnterprise = TRUE;
    else
      m_Data.m_bEnterprise = FALSE;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  if (GetOIVers() == OIVERS_NOTOI)
    m_Data.m_bEnterprise = TRUE;
  else
    m_Data.m_bEnterprise = FALSE;

  LPOBJECTLIST ls;

  // Columns
  m_Data.m_acsIdxColumns.RemoveAll();
  m_Data.m_awIdxColumns.RemoveAll();
  ls = indexparams.lpidxCols;
  while (ls) {
    LPIDXCOLS lpIdxCols = (LPIDXCOLS)ls->lpObject;
    m_Data.m_acsIdxColumns.Add((LPCTSTR)lpIdxCols->szColName);
    m_Data.m_awIdxColumns.Add( (lpIdxCols->attr.bKey)? 1: 0);
    ls = (LPOBJECTLIST)ls->lpNext;
  }
  ASSERT (m_Data.m_acsIdxColumns.GetUpperBound() != -1);
  ASSERT (m_Data.m_awIdxColumns.GetUpperBound() != -1);

  // locations
  m_Data.m_acsLocations.RemoveAll();
  ls = indexparams.lpLocations;
  while (ls) {
    m_Data.m_acsLocations.Add((LPCTSTR)ls->lpObject);
    ls = (LPOBJECTLIST)ls->lpNext;
  }
  if (m_Data.m_acsLocations.GetUpperBound() == -1) {
    CString csNoLoc;
    csNoLoc.LoadString(IDS_TM_LOCATION_NONE);// = "< No Locations >";
    m_Data.m_acsLocations.Add(csNoLoc);
  }

  m_Data.m_csSchema = indexparams.objectowner;
  m_Data.m_csParent    = indexparams.TableOwner;
  m_Data.m_csParent   += ".";
  m_Data.m_csParent   += CString(indexparams.TableName);

  m_Data.m_bStruct1 = m_Data.m_bStruct2 = FALSE;
  m_Data.m_bStruct3 = m_Data.m_bStruct4 = FALSE;
  m_Data.m_csCaptStruct1 = m_Data.m_csStruct1 = "";
  m_Data.m_csCaptStruct2 = m_Data.m_csStruct2 = "";
  m_Data.m_csCaptStruct3 = m_Data.m_csStruct3 = "";
  m_Data.m_csCaptStruct4 = m_Data.m_csStruct4 = "";
  switch(indexparams.nStructure) {
    case IDX_HASH:
      m_Data.m_csStorage = "Hash";
      m_Data.m_bStruct1 = m_Data.m_bStruct2 = TRUE;
      m_Data.m_csCaptStruct1 = "Min Pages:";
      m_Data.m_csStruct1.Format("%ld", indexparams.nMinPages);
      m_Data.m_csCaptStruct2 = "Max Pages:";
      m_Data.m_csStruct2.Format("%ld", indexparams.nMaxPages);
      break;

    case IDX_BTREE:
      m_Data.m_csStorage = "Btree";
      m_Data.m_bStruct1 = m_Data.m_bStruct2 = TRUE;
      m_Data.m_csCaptStruct1 = "Leaffill:";
      m_Data.m_csStruct1.Format("%d", indexparams.nLeafFill);
      m_Data.m_csCaptStruct2 = "NonLeaffill:";
      m_Data.m_csStruct2.Format("%d", indexparams.nNonLeafFill);
      break;

    case IDX_ISAM:
      m_Data.m_csStorage = "Isam";
      break;

    case IDX_RTREE:
      m_Data.m_csStorage = "Rtree";
      m_Data.m_bStruct1 = m_Data.m_bStruct2 = TRUE;
      m_Data.m_bStruct3 = m_Data.m_bStruct4 = TRUE;
      m_Data.m_csCaptStruct1 = "Min X:";
      m_Data.m_csStruct1 = indexparams.minX;
      m_Data.m_csCaptStruct2 = "Min Y:";
      m_Data.m_csStruct2 = indexparams.minY;
      m_Data.m_csCaptStruct3 = "Max X:";
      m_Data.m_csStruct3 = indexparams.maxX;
      m_Data.m_csCaptStruct4 = "Max Y:";
      m_Data.m_csStruct4 = indexparams.maxY;
      break;

    case IDX_STORAGE_STRUCT_UNKNOWN:
      m_Data.m_csStorage = VDBA_MfcResourceString (IDS_NOT_AVAILABLE); // "Unknown";"n/a"
      break;

    default:
      ASSERT (FALSE);
      m_Data.m_csStorage = VDBA_MfcResourceString (IDS_NOT_AVAILABLE); // "Unknown";"n/a"
      break;
  }

  switch(indexparams.nUnique) {
    case IDX_UNIQUE_NO:
      m_Data.m_csUnique = "No";
      break;
    case IDX_UNIQUE_ROW:
      m_Data.m_csUnique = "Row";
      break;
    case IDX_UNIQUE_STATEMENT:
      m_Data.m_csUnique = "Statement";
      break;
    case IDX_HEAP:
      m_Data.m_csUnique = "Heap";
      break;
    case IDX_HEAPSORT:
      m_Data.m_csUnique = "Heapsort";
      break;
    default:
      // ASSERT (FALSE);
      m_Data.m_csUnique = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);  // "Unknown";"n/a"
      break;
  }
  if (indexparams.uPage_size != -1L)
    m_Data.m_csPgSize.Format("%u", indexparams.uPage_size);
  else
    m_Data.m_csPgSize = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//"n/a"

  if (indexparams.lAllocation != -1L)
    m_Data.m_csAlloc.Format("%ld", indexparams.lAllocation);
  else
    m_Data.m_csAlloc = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//"n/a"

  if (indexparams.lExtend != -1L)
    m_Data.m_csExtend.Format("%ld", indexparams.lExtend);
  else
    m_Data.m_csExtend = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//"n/a"

  if (indexparams.lAllocatedPages != -1L)
    m_Data.m_csAllocatedPages.Format("%ld", indexparams.lAllocatedPages);
  else
    m_Data.m_csAllocatedPages = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//"n/a"

  if (indexparams.nFillFactor != -1L)
    m_Data.m_csFillFact.Format("%d", indexparams.nFillFactor);
  else
    m_Data.m_csFillFact = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);//"n/a"
  m_Data.m_bPersists = indexparams.bPersistence;
  m_Data.m_bCompression  = TRUE;
  m_Data.m_csCompress = "";
  m_Data.m_bCompress = FALSE;
  switch(indexparams.nStructure) {
    case IDX_BTREE:
      m_Data.m_csCompress.LoadString(IDS_I_KEY_COMPRESS);// = "Key Compression";
      m_Data.m_bCompress  = indexparams.bKeyCompression;
      break;

    case IDX_HASH:
    case IDX_ISAM:
    case IDX_RTREE:
      m_Data.m_csCompress.LoadString(IDS_DATA_COMPRESS);// = "Data Compression";
      m_Data.m_bCompress  = indexparams.bDataCompression;
      break;

    default:
      m_Data.m_bCompression  = FALSE;
      break;
  }

  // liberate detail structure
  FreeAttachedPointers (&indexparams, OT_INDEX);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIndex::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIndex") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIndex* pData = (CuDomPropDataIndex*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIndex)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIndex::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIndex* pData = new CuDomPropDataIndex(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIndex::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Initialize listctrl if necessary
  if (m_bMustInitialize) {
    m_bMustInitialize = FALSE;
    const int LAYOUT_NUMBER = 2;
    LV_COLUMN lvcolumn;
    TCHAR strHeader[LAYOUT_NUMBER][32];
    lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_COL_NAME));
    lstrcpy(strHeader[1], VDBA_MfcResourceString(IDS_TC_KEY));
    int i, hWidth   [LAYOUT_NUMBER] = {140, 60};
    int fmt         [LAYOUT_NUMBER] = {LVCFMT_LEFT, LVCFMT_LEFT};
    //
    memset (&lvcolumn, 0, sizeof (LV_COLUMN));
    lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
    for (i=0; i<LAYOUT_NUMBER; i++) {
      lvcolumn.fmt = fmt[i];;
      lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader[i];
      lvcolumn.iSubItem = i;
      lvcolumn.cx = hWidth[i];
      m_clistColumns.InsertColumn (i, &lvcolumn); 
    }
  }

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  // columns
  m_clistColumns.DeleteAllItems();
  size = m_Data.m_acsIdxColumns.GetSize();
  ASSERT (size == m_Data.m_awIdxColumns.GetSize());
  for (cnt = 0; cnt < size; cnt++) {
    int nCount, index;
    nCount = m_clistColumns.GetItemCount ();
    index  = m_clistColumns.InsertItem (nCount, m_Data.m_acsIdxColumns[cnt], 0);
    if (m_Data.m_awIdxColumns[cnt] != 0)
      m_clistColumns.SetItemText (index, 1, "Yes");
    else
      m_clistColumns.SetItemText (index, 1, "No");
  }

  // locations
  m_lbLocations.ResetContent();
  if (m_Data.m_bEnterprise)
    m_lbLocations.AddString(VDBA_MfcResourceString (IDS_NOT_AVAILABLE));//_T("n/a")
  else {
    size = m_Data.m_acsLocations.GetSize();
    for (cnt = 0; cnt < size; cnt++)
      m_lbLocations.AddString(m_Data.m_acsLocations[cnt]);
  }

  // schema and parent
  m_edIdxSchema.SetWindowText(m_Data.m_csSchema);
  m_edParent.SetWindowText(m_Data.m_csParent);

  // Storage Stucture
  if (m_Data.m_bEnterprise) {
    m_edStruct.SetWindowText(VDBA_MfcResourceString (IDS_NOT_AVAILABLE));//_T("n/a")
    m_stStruct1.ShowWindow(SW_HIDE);
    m_edStruct1.ShowWindow(SW_HIDE);
    m_stStruct2.ShowWindow(SW_HIDE);
    m_edStruct2.ShowWindow(SW_HIDE);
    m_stStruct3.ShowWindow(SW_HIDE);
    m_edStruct3.ShowWindow(SW_HIDE);
    m_stStruct4.ShowWindow(SW_HIDE);
    m_edStruct4.ShowWindow(SW_HIDE);
  }
  else {
    m_edStruct.SetWindowText(m_Data.m_csStorage);
    if (m_Data.m_bStruct1) {
      m_stStruct1.SetWindowText(m_Data.m_csCaptStruct1);
      m_edStruct1.SetWindowText(m_Data.m_csStruct1);
    }
    m_stStruct1.ShowWindow(m_Data.m_bStruct1? SW_SHOW : SW_HIDE);
    m_edStruct1.ShowWindow(m_Data.m_bStruct1? SW_SHOW : SW_HIDE);

    if (m_Data.m_bStruct2) {
      m_stStruct2.SetWindowText(m_Data.m_csCaptStruct2);
      m_edStruct2.SetWindowText(m_Data.m_csStruct2);
    }
    m_stStruct2.ShowWindow(m_Data.m_bStruct2? SW_SHOW : SW_HIDE);
    m_edStruct2.ShowWindow(m_Data.m_bStruct2? SW_SHOW : SW_HIDE);

    if (m_Data.m_bStruct3) {
      m_stStruct3.SetWindowText(m_Data.m_csCaptStruct3);
      m_edStruct3.SetWindowText(m_Data.m_csStruct3);
    }
    m_stStruct3.ShowWindow(m_Data.m_bStruct3? SW_SHOW : SW_HIDE);
    m_edStruct3.ShowWindow(m_Data.m_bStruct3? SW_SHOW : SW_HIDE);

    if (m_Data.m_bStruct4) {
      m_stStruct4.SetWindowText(m_Data.m_csCaptStruct4);
      m_edStruct4.SetWindowText(m_Data.m_csStruct4);
    }
    m_stStruct4.ShowWindow(m_Data.m_bStruct4? SW_SHOW : SW_HIDE);
    m_edStruct4.ShowWindow(m_Data.m_bStruct4? SW_SHOW : SW_HIDE);
  }

  // parameters
  if (m_Data.m_bEnterprise) {
    CString csNotAvailable = VDBA_MfcResourceString (IDS_NOT_AVAILABLE);
    m_edUnique.SetWindowText(csNotAvailable);//_T("n/a")
    m_edPgSize.SetWindowText(csNotAvailable);//_T("n/a")
    m_edAlloc.SetWindowText(csNotAvailable);//_T("n/a")
    m_edExtend.SetWindowText(csNotAvailable);//_T("n/a")
    m_edAllocatedPages.SetWindowText(csNotAvailable);//_T("n/a")
    m_edFillFact.SetWindowText(csNotAvailable);//_T("n/a")
    m_chkPersist.ShowWindow(SW_HIDE);
    m_stPersist.ShowWindow(SW_HIDE);
    m_stCompress.ShowWindow(SW_HIDE);
    m_chkCompress.ShowWindow(SW_HIDE);
  }
  else {
    m_edUnique.SetWindowText(m_Data.m_csUnique);
    m_edPgSize.SetWindowText(m_Data.m_csPgSize);
    m_edAlloc.SetWindowText(m_Data.m_csAlloc);
    m_edExtend.SetWindowText(m_Data.m_csExtend);
    m_edAllocatedPages.SetWindowText(m_Data.m_csAllocatedPages);
    m_edFillFact.SetWindowText(m_Data.m_csFillFact);
    m_chkPersist.SetCheck(m_Data.m_bPersists ? 1: 0);
    if (m_Data.m_bCompression) {
      m_stCompress.SetWindowText(m_Data.m_csCompress);
      m_chkCompress.SetCheck(m_Data.m_bCompress? 1: 0);
    }
    m_stCompress.ShowWindow(m_Data.m_bCompression? SW_SHOW : SW_HIDE);
    m_chkCompress.ShowWindow(m_Data.m_bCompression? SW_SHOW : SW_HIDE);
    if (m_Data.m_csPgSize == "n/a") {
      m_chkPersist.ShowWindow(SW_HIDE);
      m_stPersist.ShowWindow(SW_HIDE);
    }
    else {
      m_chkPersist.ShowWindow(SW_SHOW);
      m_stPersist.ShowWindow(SW_SHOW);
    }
  }
}

void CuDlgDomPropIndex::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_lbLocations.ResetContent();
  m_edIdxSchema.SetWindowText(_T(""));
  m_edParent.SetWindowText(_T(""));
  m_edStruct.SetWindowText(_T(""));
  m_stStruct1.SetWindowText(_T(""));
  m_edStruct1.SetWindowText(_T(""));
  m_stStruct2.SetWindowText(_T(""));
  m_edStruct2.SetWindowText(_T(""));
  m_stStruct3.SetWindowText(_T(""));
  m_edStruct3.SetWindowText(_T(""));
  m_stStruct4.SetWindowText(_T(""));
  m_edStruct4.SetWindowText(_T(""));
  m_edUnique.SetWindowText(_T(""));
  m_edPgSize.SetWindowText(_T(""));
  m_edAlloc.SetWindowText(_T(""));
  m_edExtend.SetWindowText(_T(""));
  m_edAllocatedPages.SetWindowText(_T(""));
  m_edFillFact.SetWindowText(_T(""));
  m_chkPersist.SetCheck(0);
  m_stCompress.SetWindowText(_T(""));
  m_chkCompress.SetCheck(0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
