/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdptbpg.cpp : implementation file
**    Project  : vdba
**    Author   : Emannuel Blattes
**    Purpose  : Page of Table 
**
** History:
** 31-Dec-1999 (uk$so01)
*     Add function CopyData()
** 10-May-2001 (noifr01)
**   (bug 104694) 3 member variables weren't copied in the CopyData() method
** 04-Jul-2002 (uk$so01)
**    BUG #108183, DOM/Table/Page right pane flashs on certain OS with 256 colors.
**    For example Windows 2000 and Windows 95.
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdptbpg.h"
#include "vtree.h"

#include "dgerrh.h"     // MessageWithHistoryButton

extern "C" {
  #include "main.h"
  #include "dom.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   2


CuDlgDomPropTblPages::CuDlgDomPropTblPages(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTblPages::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropTblPages)
	m_strAvgLengOverflowChain = _T("");
	m_strLongestOverflowChain = _T("");
	m_strTotal = _T("");
	m_strDataBucket = _T("");
	m_strEmptyBucket = _T("");
	m_strDataWithOverflow = _T("");
	m_strEmptyWithOverflow = _T("");
	m_strOverflow = _T("");
	m_strEmptyOverflow = _T("");
	m_strNeverUsed = _T("");
	m_strFree = _T("");
	m_strOthers = _T("");
	//}}AFX_DATA_INIT
	m_pPieCtrl = NULL;
	m_pPieCtrlHashTable = NULL;
	m_zeroPctColor  = RGB(0, 0, 0);
	m_crDataBucket = RGB (0, 0, 255);
	m_crEmptyBucket = RGB (0, 255, 255);
	m_crDataWithOverflow = RGB (0, 0, 255);
	m_crEmptyWithOverflow = RGB (0, 255, 255);
	m_crOverflow = RGB (255, 0, 0);
	m_crEmptyOverflow = RGB (255, 0, 255);
	m_crNeverUsed = RGB (255, 255, 255);
	m_crFreed = RGB (255, 255, 0);
	m_crOthers = RGB (193, 192, 192);

	// Emb overrides colors to shades of blue to be consistent with cell pages colors
	m_crDataBucket = RGB (146, 189, 224) ,        // luminance 174
	m_crEmptyBucket = RGB (200, 222, 240) ,       // luminance 207
	m_crDataWithOverflow = RGB (146, 189, 224) ,  // luminance 174
	m_crEmptyWithOverflow = RGB (200, 222, 240) , // luminance 207

	m_Data.m_refreshParams.InitializeRefreshParamsType(OT_TABLE);

	CuCellCtrl::CreateMetalBluePalette(&m_MetalBluePalette);
	CuCellCtrl::CreateDetailedPalette(&m_DetailedPalette);
	if ((HPALETTE)m_MetalBluePalette != NULL)
		m_cListCtrl.SetMetalBluePalette(&m_MetalBluePalette);
	if ((HPALETTE)m_DetailedPalette != NULL)
		m_cListCtrl.SetDetailPalette(&m_DetailedPalette);
}

void CuDlgDomPropTblPages::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTblPages)
  DDX_Control(pDX, IDC_STATIC_PGPERCELL, m_stRatio);
  DDX_Control(pDX, IDC_STATIC_PCT, m_stPct);
  DDX_Control(pDX, IDC_COMBO_RATIO, m_cbRatio);
  DDX_Control(pDX, IDC_STATIC_ZEROPCT, m_stZeroPct);
  DDX_Control(pDX, IDC_STATIC_PG1,    m_stPg1 );
  DDX_Control(pDX, IDC_STATIC_PG2,    m_stPg2 );
  DDX_Control(pDX, IDC_STATIC_PG3,    m_stPg3 );
  DDX_Control(pDX, IDC_STATIC_PG4,    m_stPg4 );
  DDX_Control(pDX, IDC_STATIC_PG5,    m_stPg5 );
  DDX_Control(pDX, IDC_STATIC_PG6,    m_stPg6 );
  DDX_Control(pDX, IDC_STATIC_PG7,    m_stPg7 );
  DDX_Control(pDX, IDC_STATIC_PG8,    m_stPg8 );
  DDX_Control(pDX, IDC_STATIC_PG9,    m_stPg9 );
  DDX_Control(pDX, IDC_STATIC_PG10,   m_stPg10 );
  DDX_Control(pDX, IDC_STATIC_PG11,   m_stPg11 );
  DDX_Control(pDX, IDC_STATIC_PG12,   m_stPg12 );
	DDX_Text(pDX, IDC_STATIC_V_ALOC, m_strAvgLengOverflowChain);
	DDX_Text(pDX, IDC_STATIC_V_LOC, m_strLongestOverflowChain);
	DDX_Text(pDX, IDC_STATIC_V_TOTAL, m_strTotal);
	DDX_Text(pDX, IDC_STATIC_V1, m_strDataBucket);
	DDX_Text(pDX, IDC_STATIC_V2, m_strEmptyBucket);
	DDX_Text(pDX, IDC_STATIC_V3, m_strDataWithOverflow);
	DDX_Text(pDX, IDC_STATIC_V4, m_strEmptyWithOverflow);
	DDX_Text(pDX, IDC_STATIC_V5, m_strOverflow);
	DDX_Text(pDX, IDC_STATIC_V6, m_strEmptyOverflow);
	DDX_Text(pDX, IDC_STATIC_V7, m_strNeverUsed);
	DDX_Text(pDX, IDC_STATIC_V8, m_strFree);
	DDX_Text(pDX, IDC_STATIC_V9, m_strOthers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTblPages, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropTblPages)
    ON_WM_SIZE()
  ON_WM_CTLCOLOR()
  ON_CBN_SELCHANGE(IDC_COMBO_RATIO, OnSelchangeComboRatio)
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
    ON_NOTIFY(TTN_NEEDTEXT, NULL, OnTooltipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTblPages message handlers

void CuDlgDomPropTblPages::OnTooltipText(NMHDR* pnmh, LRESULT* pResult)
{
  TOOLTIPTEXT* pttt = (TOOLTIPTEXT*) pnmh;
  lstrcpy(pttt->szText, m_cListCtrl.GetShortTooltipText());
}

void CuDlgDomPropTblPages::PostNcDestroy() 
{
	if ((HPALETTE)m_MetalBluePalette != NULL)
		m_MetalBluePalette.DeleteObject();
	if ((HPALETTE)m_DetailedPalette != NULL)
		m_DetailedPalette.DeleteObject();
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTblPages::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Branch custom control
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

	// supply number of cells on one line - MANDATORY
	m_cListCtrl.InitializeCells(50);

	// Get colors that will be used for custom controls
	m_zeroPctColor = CuCellCtrl::GetZeroPctColor();
	m_zeroPctBrush.CreateSolidBrush(m_zeroPctColor);

	if ((HPALETTE)m_DetailedPalette != NULL)
	{
		CWnd* array[13] = 
		{
			&m_st100Pct,
			&m_stCell1,
			&m_stCell2,
			&m_stCell3,
			&m_stCell4,
			&m_stCell5,
			&m_stCell6,
			&m_stCell7,
			&m_stCell8,
			&m_stCell9,
			&m_stCell10,
			&m_stCell11,
			&m_stCell12
		};

		CWnd* array2[9] = 
		{
			&m_crI1,
			&m_crI2,
			&m_crI3,
			&m_crI4,
			&m_crI5,
			&m_crI6,
			&m_crI7,
			&m_crI8,
			&m_crI9
		};
	
		int i;
		for (i=0; i<13; i++)
		{
			if (array[i])
			{
				((CuStaticCellItem*)array[i])->SetPalette(&m_DetailedPalette);
			}
		}
		for (i=0; i<9; i++)
		{
			if (array2[i])
			{
				((CuStaticColorWnd*)array2[i])->SetPaletteAware(TRUE);
				((CuStaticColorWnd*)array2[i])->SetPalette(&m_DetailedPalette);
			}
		}
	}

	VERIFY (m_st100Pct.SubclassDlgItem(IDC_STATIC_100PCT, this));

	VERIFY (m_stCell1.SubclassDlgItem(  IDC_STATIC_CELL1,  this));
	VERIFY (m_stCell2.SubclassDlgItem(  IDC_STATIC_CELL2,  this));
	VERIFY (m_stCell3.SubclassDlgItem(  IDC_STATIC_CELL3,  this));
	VERIFY (m_stCell4.SubclassDlgItem(  IDC_STATIC_CELL4,  this));
	VERIFY (m_stCell5.SubclassDlgItem(  IDC_STATIC_CELL5,  this));
	VERIFY (m_stCell6.SubclassDlgItem(  IDC_STATIC_CELL6,  this));
	VERIFY (m_stCell7.SubclassDlgItem(  IDC_STATIC_CELL7,  this));
	VERIFY (m_stCell8.SubclassDlgItem(  IDC_STATIC_CELL8,  this));
	VERIFY (m_stCell9.SubclassDlgItem(  IDC_STATIC_CELL9,  this));
	VERIFY (m_stCell10.SubclassDlgItem( IDC_STATIC_CELL10, this));
	VERIFY (m_stCell11.SubclassDlgItem( IDC_STATIC_CELL11, this));
	VERIFY (m_stCell12.SubclassDlgItem( IDC_STATIC_CELL12, this));

	// Set cells legends according to default setting (non-hashed table)
	UpdateCellLegends(m_Data.m_bHashed);

	//
	// Create Pie Control:
	m_crI1.SetColor (m_crDataBucket);
	m_crI2.SetColor (m_crEmptyBucket);
	m_crI3.SetColor (m_crDataWithOverflow,  RGB (255, 0, 0));
	m_crI4.SetColor (m_crEmptyWithOverflow, RGB (255, 0, 0));
	m_crI5.SetColor (m_crOverflow);
	m_crI6.SetColor (m_crEmptyOverflow);
	m_crI7.SetColor (m_crNeverUsed);
	m_crI8.SetColor (m_crFreed);
	m_crI9.SetColor (m_crOthers);

	VERIFY (m_crI1.SubclassDlgItem (IDC_STATIC_I1, this));
	VERIFY (m_crI2.SubclassDlgItem (IDC_STATIC_I2, this));
	VERIFY (m_crI3.SubclassDlgItem (IDC_STATIC_I3, this));
	VERIFY (m_crI4.SubclassDlgItem (IDC_STATIC_I4, this));
	VERIFY (m_crI5.SubclassDlgItem (IDC_STATIC_I5, this));
	VERIFY (m_crI6.SubclassDlgItem (IDC_STATIC_I6, this));
	VERIFY (m_crI7.SubclassDlgItem (IDC_STATIC_I7, this));
	VERIFY (m_crI8.SubclassDlgItem (IDC_STATIC_I8, this));
	VERIFY (m_crI9.SubclassDlgItem (IDC_STATIC_I9, this));

	CreatePieChartCtrl();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTblPages::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;

  CRect rDlg, r;
  GetClientRect (rDlg);

  // adjust width and height of matrix control
  m_cListCtrl.GetWindowRect (r);
  ScreenToClient (r);
  r.bottom = rDlg.bottom - r.left;
  r.right = rDlg.right - r.left;
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropTblPages::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropTblPages::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get pages of table, even if system object (lpRecord->ownerName is "$ingres" )
  //
  INGRESPAGEINFO pageinfo;
  memset(&pageinfo, 0, sizeof(INGRESPAGEINFO));
  BOOL bOK = GetIngresPageInfo(nNodeHandle, lpRecord->extra, lpRecord->objName, lpRecord->ownerName, &pageinfo);
  if (!bOK) {
    // Need to reset pageinfo since has been partially filled by GetIngresPageInfo
    memset(&pageinfo, 0, sizeof(INGRESPAGEINFO));
    // "Cannot Access Page Information"
    MessageWithHistoryButton(m_hWnd, VDBA_MfcResourceString (IDS_E_PAGE_INFORMATION));
  }

  ASSERT (m_pPieCtrl && m_pPieCtrlHashTable);
  if (m_pPieCtrl && m_pPieCtrlHashTable)
  {
     //
     // Hash Table or Non-Hash Table. This value should be changed
     // upon the table's structure:
   //  BOOL bHashTable = TRUE; 
     ShowPieChartCtrl ((LPVOID)&pageinfo, pageinfo.bNewHash);

  /*
    double dPercent = 0.0;
    CString strTitle;
    CaPieInfoData* pData = m_pPieCtrl->GetPieInformation();
    pData->Cleanup();
    pData->SetCapacity ((double)pageinfo.ltotal, _T(""));
    if (pageinfo.linuse > 0)
    {
        dPercent = ((double)pageinfo.linuse / (double)pageinfo.ltotal) * 100.00;
        pData->AddPie2 (_T("In Use"), dPercent, RGB(0, 0, 192), (double)pageinfo.linuse);
    }
    else
        pData->AddLegend (_T("In Use"), RGB(0, 0, 192));

    if (pageinfo.lfreed > 0)
    {
        dPercent = ((double)pageinfo.lfreed / (double)pageinfo.ltotal) * 100.00;
        pData->AddPie2 (_T("Freed"), dPercent, RGB(0, 192, .0), (double)pageinfo.lfreed);
    }
    else
        pData->AddLegend (_T("Freed"), RGB(0, 192, .0));

    if (pageinfo.lneverused > 0)
    {
        dPercent = ((double)pageinfo.lneverused / (double)pageinfo.ltotal) * 100.00;
        pData->AddPie2 (_T("Never Used"), dPercent, RGB(255, 255, 0), (double)pageinfo.lneverused);
    }
    else
        pData->AddLegend (_T("Never Used"), RGB(255, 255, 0));

    if (pageinfo.loverflow > 0)
    {
        dPercent = ((double)pageinfo.loverflow / (double)pageinfo.ltotal) * 100.00;
        pData->AddPie2 (_T("Overflow"),  dPercent, RGB(255, 0, 0), (double)pageinfo.lneverused);
    }
    else
        pData->AddLegend (_T("Overflow"), RGB(255, 0, 0));

    strTitle.Format (_T("Total = %d"), pageinfo.ltotal);
    pData->SetTitle(strTitle);
    m_pPieCtrl->UpdateLegend();
    m_pPieCtrl->UpdatePieChart();
   */
  }
  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  CopyData (&m_Data, &pageinfo, TRUE); // pageinfo -> m_Data

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTblPages::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (_tcscmp (pClass, _T("CuDomPropDataPages")) == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataPages* pData = (CuDomPropDataPages*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataPages)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // flag loaded object that it must delete the array itself
  pData->SetMustDeleteArray();

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropTblPages::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataPages* pData = new CuDomPropDataPages(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropTblPages::RefreshDisplay(INGRESPAGEINFO* pPageInfo)
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_cListCtrl.ShowWindow(SW_SHOW);   // can have been hidden by ResetDisplay()

  INGRESPAGEINFO pageinfo;
  CopyData (&m_Data, &pageinfo, FALSE); // m_Data -> pageinfo
  ShowPieChartCtrl ((LPVOID)&pageinfo, pageinfo.bNewHash);

  // MANAGE NOT DISPLAYABLE CASE ($SYSTEM AS OWNER, ETC...
  if (m_Data.m_pByteArray == NULL) {
    m_stRatio.ShowWindow   (SW_HIDE);
    m_cbRatio.ShowWindow   (SW_HIDE);
    m_stPct.ShowWindow     (SW_HIDE);
    m_stZeroPct.ShowWindow (SW_HIDE);
    m_st100Pct.ShowWindow  (SW_HIDE);
    HidePageTypeControls();
    m_cListCtrl.SetDataForDisplay(m_Data.m_lItemsPerByte,
                                  m_Data.m_nbElements, m_Data.m_pByteArray,
                                  m_Data.m_lCurPgPerCell);

    CString csValue(VDBA_MfcResourceString (IDS_NOT_AVAILABLE));//_T("N/A")
    GetDlgItem(IDC_EDIT_PAGES)->SetWindowText(csValue);
    GetDlgItem(IDC_EDIT_INUSE)->SetWindowText(csValue);
    GetDlgItem(IDC_EDIT_OVERFLOW)->SetWindowText(csValue);
    GetDlgItem(IDC_EDIT_FREED)->SetWindowText(csValue);
    GetDlgItem(IDC_EDIT_NEVERUSED)->SetWindowText(csValue);

    return;
  }

  // in case previously hidden
  m_stRatio.ShowWindow   (SW_SHOW);
  m_cbRatio.ShowWindow   (SW_SHOW);

  // fill combobox with set of values
  int goodSel = CB_ERR;
  m_cbRatio.ResetContent();
  CString csCbItem;
  csCbItem.Format(_T("%ld"), m_Data.m_lItemsPerByte);   // Most detailed possible
  int index = m_cbRatio.AddString(csCbItem);
  m_cbRatio.SetItemData(index, m_Data.m_lItemsPerByte);
  if (m_Data.m_lItemsPerByte == m_Data.m_lCurPgPerCell)
    goodSel = index;
  for (long lPgPerCell = m_Data.m_lItemsPerByte * 10;
       (m_Data.m_nbElements * m_Data.m_lItemsPerByte) * 10 > lPgPerCell;   // stop as soon as fits in only one cell
       lPgPerCell *= 10) {
    csCbItem.Format(_T("%ld"), lPgPerCell);
    index = m_cbRatio.AddString(csCbItem);
    m_cbRatio.SetItemData(index, lPgPerCell);
    if (lPgPerCell == m_Data.m_lCurPgPerCell)
      goodSel = index;
  }

  // Selection on "current number of pages per cell"
  ASSERT (goodSel != CB_ERR);     
  m_cbRatio.SetCurSel(goodSel);

  // Show/hide 0% - 100% cells indicators according to current number of pages per cell
  m_stPct.ShowWindow     ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  m_stZeroPct.ShowWindow ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  m_st100Pct.ShowWindow  ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  if (m_Data.m_lCurPgPerCell == 1)
    ShowPageTypeControls();
  else
    HidePageTypeControls();

  // display cells - using current number of displayed pages per cell
  ASSERT (m_Data.m_lCurPgPerCell % m_Data.m_lItemsPerByte == 0);
  m_cListCtrl.SetDataForDisplay(m_Data.m_lItemsPerByte,
                                m_Data.m_nbElements, m_Data.m_pByteArray,
                                m_Data.m_lCurPgPerCell);

  CString csValue;
  csValue.Format(_T("%ld"), m_Data.m_lTotal);
  GetDlgItem(IDC_EDIT_PAGES)->SetWindowText(csValue);
  csValue.Format(_T("%ld"), m_Data.m_lInUse);
  GetDlgItem(IDC_EDIT_INUSE)->SetWindowText(csValue);
  csValue.Format(_T("%ld"), m_Data.m_lOverflow);
  GetDlgItem(IDC_EDIT_OVERFLOW)->SetWindowText(csValue);
  csValue.Format(_T("%ld"), m_Data.m_lFreed);
  GetDlgItem(IDC_EDIT_FREED)->SetWindowText(csValue);
  csValue.Format(_T("%ld"), m_Data.m_lNeverUsed);
  GetDlgItem(IDC_EDIT_NEVERUSED)->SetWindowText(csValue);

}


HBRUSH CuDlgDomPropTblPages::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
  HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

  if (nCtlColor == CTLCOLOR_STATIC) {
    if (pWnd->m_hWnd == m_stZeroPct.m_hWnd) {
      // black on white
      pDC->SetTextColor(RGB(0, 0, 0));
      pDC->SetBkColor(m_zeroPctColor);
      return (HBRUSH)m_zeroPctBrush;
    }
  }

  // default case
  return hbr;
}


void CuDlgDomPropTblPages::OnSelchangeComboRatio() 
{
  // Pick "current number of pages per cell"
  int index = m_cbRatio.GetCurSel();
  ASSERT (index != CB_ERR);
  long lCurPgPerCell = m_cbRatio.GetItemData(index);
  ASSERT (lCurPgPerCell > 0);
  m_Data.m_lCurPgPerCell = lCurPgPerCell;

  // Show/hide 0% - 100% cells indicators according to current number of pages per cell
  m_stPct.ShowWindow     ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  m_stZeroPct.ShowWindow ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  m_st100Pct.ShowWindow  ( (m_Data.m_lCurPgPerCell == 1) ? SW_HIDE : SW_SHOW);
  if (m_Data.m_lCurPgPerCell == 1)
    ShowPageTypeControls();
  else
    HidePageTypeControls();

  // display cells - using current number of displayed pages per cell
  ASSERT (m_Data.m_lCurPgPerCell % m_Data.m_lItemsPerByte == 0);
  m_cListCtrl.DisplayAllLines(m_Data.m_lCurPgPerCell);
}

void CuDlgDomPropTblPages::ShowPageTypeControls()
{
  SetPageTypeControlsVisibility(SW_SHOW);
}

void CuDlgDomPropTblPages::HidePageTypeControls()
{
  SetPageTypeControlsVisibility(SW_HIDE);
}

void CuDlgDomPropTblPages::SetPageTypeControlsVisibility(int nCmdShow)
{
  ASSERT (nCmdShow == SW_SHOW || nCmdShow == SW_HIDE);

  m_stCell1.ShowWindow(nCmdShow);
  m_stPg1.ShowWindow(nCmdShow);
  m_stCell2.ShowWindow(nCmdShow);
  m_stPg2.ShowWindow(nCmdShow);
  m_stCell3.ShowWindow(nCmdShow);
  m_stPg3.ShowWindow(nCmdShow);
  m_stCell4.ShowWindow(nCmdShow);
  m_stPg4.ShowWindow(nCmdShow);
  m_stCell5.ShowWindow(nCmdShow);
  m_stPg5.ShowWindow(nCmdShow);
  m_stCell6.ShowWindow(nCmdShow);
  m_stPg6.ShowWindow(nCmdShow);
  m_stCell7.ShowWindow(nCmdShow);
  m_stPg7.ShowWindow(nCmdShow);
  m_stCell8.ShowWindow(nCmdShow);
  m_stPg8.ShowWindow(nCmdShow);
  m_stCell9.ShowWindow(nCmdShow);
  m_stPg9.ShowWindow(nCmdShow);
  m_stCell10.ShowWindow(nCmdShow);
  m_stPg10.ShowWindow(nCmdShow);
  m_stCell11.ShowWindow(nCmdShow);
  m_stPg11.ShowWindow(nCmdShow);
  m_stCell12.ShowWindow(nCmdShow);
  m_stPg12.ShowWindow(nCmdShow);

  if (nCmdShow == SW_SHOW)
    UpdateCellLegends(m_Data.m_bHashed);
}

void CuDlgDomPropTblPages::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  GetDlgItem(IDC_EDIT_PAGES)->SetWindowText(_T(""));
  GetDlgItem(IDC_EDIT_INUSE)->SetWindowText(_T(""));
  GetDlgItem(IDC_EDIT_OVERFLOW)->SetWindowText(_T(""));
  GetDlgItem(IDC_EDIT_FREED)->SetWindowText(_T(""));
  GetDlgItem(IDC_EDIT_NEVERUSED)->SetWindowText(_T(""));
  m_cbRatio.ResetContent();
  m_cListCtrl.ShowWindow(SW_HIDE);   // needs to be displayed again in RefreshDisplay()
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

void CuDlgDomPropTblPages::CreatePieChartCtrl()
{
	CRect r;
	CaPieChartCreationData crData;
	crData.m_bDBLocation = FALSE;
	crData.m_bUseLButtonDown = FALSE;
	crData.m_bLegendRight = TRUE;
	crData.m_nColumns = 2;
	crData.m_bShowValue = TRUE;
	crData.m_bValueInteger = TRUE;
	GetDlgItem (IDC_MFC_STATIC1)->GetWindowRect (r);
	ScreenToClient (r);
	m_pPieCtrl = new CfStatisticFrame(CfStatisticFrame::nTypePie);
	m_pPieCtrl->Create(this, r, &crData);
	m_pPieCtrl->ShowWindow (SW_HIDE);

	crData.m_bUseLButtonDown = FALSE; // Do not use popup info
	crData.m_bShowValue = TRUE;       // The value is appanded to the legend item
	//
	// Legend Box:
	crData.m_bUseLegend   = FALSE;

	crData.m_b3D = TRUE;
	crData.m_bShowBarTitle = FALSE;
	crData.m_bDrawAxis = FALSE;
	crData.m_strBarWidth = _T("aaa");
	if ((HPALETTE)m_DetailedPalette != NULL)
		crData.m_pPalette = &m_DetailedPalette;

	CRect rx;
	m_crI1.GetWindowRect (rx);
	ScreenToClient(rx);

	r.right = rx.left - 10;
	m_pPieCtrlHashTable = new CfStatisticFrame(CfStatisticFrame::nTypeBar);
	m_pPieCtrlHashTable->Create(this, r, &crData);
	m_pPieCtrlHashTable->ShowWindow (SW_HIDE);
}

void CuDlgDomPropTblPages::ShowPieChartCtrl(LPVOID lpData, BOOL bHashTable)
{
	ASSERT (lpData);
	if (!lpData)
		return;
	int i;
	CWnd* pWnd = NULL;
	INGRESPAGEINFO* pPageInfo = (INGRESPAGEINFO*)lpData;
	UINT nTabStaticID[] = 
	{
		IDC_STATIC_I1,IDC_STATIC_L1,IDC_STATIC_V1,
		IDC_STATIC_I2,IDC_STATIC_L2,IDC_STATIC_V2,
		IDC_STATIC_I3,IDC_STATIC_L3,IDC_STATIC_V3,
		IDC_STATIC_I4,IDC_STATIC_L4,IDC_STATIC_V4,
		IDC_STATIC_I5,IDC_STATIC_L5,IDC_STATIC_V5,
		IDC_STATIC_I6,IDC_STATIC_L6,IDC_STATIC_V6,
		IDC_STATIC_I7,IDC_STATIC_L7,IDC_STATIC_V7,
//		IDC_STATIC_I8,IDC_STATIC_L8,IDC_STATIC_V8,
		IDC_STATIC_I9,IDC_STATIC_L9,IDC_STATIC_V9,
		IDC_STATIC_L_TOTAL,
		IDC_STATIC_L_LONGEST_OC,
		IDC_STATIC_L_AVG_OC,
		IDC_STATIC_V_TOTAL,
		IDC_STATIC_V_LOC,
		IDC_STATIC_V_ALOC,
		0
	};

	if (bHashTable) 
		GetDlgItem(IDC_MFC_STATIC_PAGECOUNT)->SetWindowText(VDBA_MfcResourceString(IDS_I_HASH_TABLE_PAGE));//_T("Hash Table Page Counts")
	else
		GetDlgItem(IDC_MFC_STATIC_PAGECOUNT)->SetWindowText(VDBA_MfcResourceString(IDS_I_PAGE_COUNT));//_T("Page Counts")

	if (bHashTable)
	{
		char buffer[30];
		double tot1, tot2, tot3, tot4, v1, v2, v3,like0;
		long lothers;
		if (m_pPieCtrl && IsWindow (m_pPieCtrl->m_hWnd))
			m_pPieCtrl->ShowWindow (SW_HIDE);
		if (m_pPieCtrlHashTable && IsWindow (m_pPieCtrlHashTable->m_hWnd))
			m_pPieCtrlHashTable->ShowWindow (SW_SHOW);
		for (i=0; nTabStaticID[i] > 0; i++)
		{
			pWnd = GetDlgItem (nTabStaticID[i]);
			if (pWnd)
				pWnd->ShowWindow (SW_SHOW);
		}
		//
		// Stack Bars:
		CaBarInfoData* pData = m_pPieCtrlHashTable->GetBarInformation();
		pData->Cleanup();

		//
		// Define the set of stack lebels.

		// Data bucket:
		pData->AddBar (_T("S01"), m_crDataBucket); 
		// Data with overflow:
		pData->AddBar (_T("H01"), m_crDataWithOverflow, TRUE, RGB (255, 0, 0)); // special label
		// Empty bucket
		pData->AddBar (_T("S02"), m_crEmptyBucket);
		// Empty with overflow
		pData->AddBar (_T("H02"), m_crEmptyWithOverflow, TRUE, RGB (255, 0, 0)); // special label
		// Overflow:
		pData->AddBar (_T("S03") ,m_crOverflow);
		// Empty overflow:
		pData->AddBar (_T("S04") ,m_crEmptyOverflow);
		// Never used:
		pData->AddBar (_T("S05") ,m_crNeverUsed);
		// Freed:
//		pData->AddBar ("S06" ,m_crFreed);
		// Other:
		pData->AddBar (_T("S07") ,m_crOthers);

		//
		// Must Add the Bar's Capacity (Tptal for each bar):

		lothers =		pPageInfo->ltotal 
						- pPageInfo->lneverused
						-pPageInfo->lbuckets_no_ovf    - pPageInfo->lemptybuckets_no_ovf
						-pPageInfo->lbuck_with_ovf     - pPageInfo->lemptybuckets_with_ovf
						-pPageInfo->loverflow_not_empty- pPageInfo->loverflow_empty;
		if (lothers<0L)
			lothers=0L;

		tot1 = (double)(pPageInfo->lbuckets_no_ovf    + pPageInfo->lemptybuckets_no_ovf);
		tot2 = (double)(pPageInfo->lbuck_with_ovf     + pPageInfo->lemptybuckets_with_ovf);
		tot3 = (double)(pPageInfo->loverflow_not_empty+ pPageInfo->loverflow_empty);
		tot4 = (double)(lothers /*+pPageInfo->lfreed*/ + pPageInfo->lneverused);
		v1=tot1;
		if (tot2>v1)
			v1=tot2;
		if (tot3>v1)
			v1=tot3;
		if (tot4>v1)
			v1=tot4;
		if (v1<4.)
			v1=4.;
		like0=v1*.03;
		if (tot1<like0)
			tot1=like0;
		if (tot2<like0)
			tot2=like0;
		if (tot3<like0)
			tot3=like0;
		if (tot4<like0)
			tot4=like0;
		like0*=1.0001;



		pData->AddBarUnit (_T("B1:"), tot1 ); // First bar
		pData->AddBarUnit (_T("B2:"), tot2 ); // Second bar
		pData->AddBarUnit (_T("B3:"), tot3 ); // Third bar
		pData->AddBarUnit (_T("B4:"), tot4 ); // Fourth bar
		
		//
		// Add the Occupants (stacks) of the bars.
		// You should round the percentage to the integer, so that the
		// picture will be drawn correctly. The sum of percentages of all Stacks
		// must be equal to 100.

		if (tot1<=like0) {
			pData->AddOccupant (_T("B1:"), _T("S01"), 50.0);
			pData->AddOccupant (_T("B1:"), _T("S02"), 50.0);
		}
		else {
			v1 = 100.0 * (double)(pPageInfo->lbuckets_no_ovf) / tot1;
			v2 = 100.0 - v1;

			pData->AddOccupant (_T("B1:"), _T("S01"), v1);
			pData->AddOccupant (_T("B1:"), _T("S02"), v2);
		}
		
		if (tot2<=like0) {
			pData->AddOccupant (_T("B2:"), _T("H01"), 50.0);
			pData->AddOccupant (_T("B2:"), _T("H02"), 50.0);
		}
		else {
			v1 = 100.0 * (double)(pPageInfo->lbuck_with_ovf) / tot2;
			v2 = 100.0 - v1;
			pData->AddOccupant (_T("B2:"), _T("H01"), v1);
			pData->AddOccupant (_T("B2:"), _T("H02"), v2);

		}
		
		if (tot3<=like0) {
			pData->AddOccupant (_T("B3:"), _T("S03"), 50.0);
			pData->AddOccupant (_T("B3:"), _T("S04"), 50.0);
		}
		else {
			v1 = 100.0 * (double)(pPageInfo->loverflow_not_empty) / tot3;
			v2 = 100.0 - v1;
			pData->AddOccupant (_T("B3:"), _T("S03"), v1);
			pData->AddOccupant (_T("B3:"), _T("S04"), v2);
		}
		
		if (tot4<=like0) {
			pData->AddOccupant (_T("B4:"), _T("S07"), 50.0);
			//pData->AddOccupant (_T("B4:"), _T("S06"), 30.0);
			pData->AddOccupant (_T("B4:"), _T("S05"), 50.0);
		}
		else {
			v1 = 100.0 * (double)(lothers)           / tot4;
//			v2 = 100.0 * (double)(pPageInfo->lfreed) / tot4;
			v3 = 100.0 - v1 ; /*-v2;*/
			pData->AddOccupant (_T("B4:"), _T("S07"), v1);
//			pData->AddOccupant (_T("B4:"), _T("S06"), v2);
			pData->AddOccupant (_T("B4:"), _T("S05"), v3);
		}

		m_strDataBucket        = ltoa(pPageInfo->lbuckets_no_ovf        ,buffer,10);
		m_strEmptyBucket       = ltoa(pPageInfo->lemptybuckets_no_ovf   ,buffer,10);
		m_strDataWithOverflow  = ltoa(pPageInfo->lbuck_with_ovf         ,buffer,10);
		m_strEmptyWithOverflow = ltoa(pPageInfo->lemptybuckets_with_ovf ,buffer,10);
		m_strOverflow          = ltoa(pPageInfo->loverflow_not_empty    ,buffer,10);
		m_strEmptyOverflow     = ltoa(pPageInfo->loverflow_empty        ,buffer,10);
		m_strNeverUsed         = ltoa(pPageInfo->lneverused             ,buffer,10);
		m_strFree              = ltoa(pPageInfo->lfreed                 ,buffer,10);
		m_strOthers            = ltoa(lothers                           ,buffer,10);
		if (pPageInfo->llongest_ovf_chain<0L)
			m_strLongestOverflowChain.LoadString (IDS_NOT_AVAILABLE);//_T("N/A")
		else
			m_strLongestOverflowChain = ltoa(pPageInfo->llongest_ovf_chain  ,buffer,10);
		if ( pPageInfo->favg_ovf_chain<0.0)
			m_strAvgLengOverflowChain.LoadString (IDS_NOT_AVAILABLE);//_T("N/A")
		else {
			sprintf (buffer, _T("%.6g"), pPageInfo->favg_ovf_chain);
			m_strAvgLengOverflowChain = buffer;
		}
		m_strTotal =ltoa(pPageInfo->ltotal,buffer,10);
	}
	else
	{
		if (m_pPieCtrlHashTable && IsWindow (m_pPieCtrlHashTable->m_hWnd))
			m_pPieCtrlHashTable->ShowWindow (SW_HIDE);
		if (m_pPieCtrl && IsWindow (m_pPieCtrl->m_hWnd))
			m_pPieCtrl->ShowWindow (SW_SHOW);
		for (i=0; nTabStaticID[i] > 0; i++)
		{
			pWnd = GetDlgItem (nTabStaticID[i]);
			if (pWnd)
				pWnd->ShowWindow (SW_HIDE);
		}

		//
		// There is a BUG. already reported to FNN:
		// Sometime 'pPageInfo->ltotal' is less than the sum of
		// 'pPageInfo->linuse', 'pPageInfo->lfreed', ...
		double dPercent = 0.0;
		CString strTitle;
		CaPieInfoData* pData = m_pPieCtrl->GetPieInformation();
		pData->Cleanup();
		pData->SetCapacity ((double)pPageInfo->ltotal, _T(""));
		long l1 = pPageInfo->linuse - pPageInfo->loverflow;  // not overflow
		long l2 = pPageInfo->loverflow;                      // overflow
		long l3 = pPageInfo->lfreed;                         // freed
		long l4 = pPageInfo->lneverused;                     // neverused
		long lt = l1+l2+l3+l4;
		if (lt == 0L) // 
			lt=1L;
		ASSERT (lt == pPageInfo->ltotal);

		if (l1 > 0)  // not overflow
		{
			dPercent = ((double)l1 / (double) lt) * 100.00;
			pData->AddPie2 (VDBA_MfcResourceString(IDS_I_NOT_OVERFLOW), dPercent, RGB(0, 0, 192), (double)l1);//_T("Not Overflow")
		}
		else
			pData->AddLegend (VDBA_MfcResourceString(IDS_I_NOT_OVERFLOW), RGB(0, 0, 192)); //_T("Not Overflow")
		
		if (l2 > 0) // overflow
		{
			dPercent = ((double)l2 / (double) lt) * 100.00;
			pData->AddPie2 (VDBA_MfcResourceString(IDS_I_OVERFLOW), dPercent, RGB(255, 0, 0), (double)l2);//_T("Overflow")
		}
		else
			pData->AddLegend (VDBA_MfcResourceString(IDS_I_OVERFLOW), RGB(255, 0, 0));//_T("Overflow")
		
		if (l3 > 0) // freed
		{
			dPercent = ((double)l3 / (double) lt) * 100.00;
			pData->AddPie2 (VDBA_MfcResourceString(IDS_I_FREED), dPercent, RGB(0, 192, .0), (double)l3);//_T("Freed")
		}
		else
			pData->AddLegend (VDBA_MfcResourceString(IDS_I_FREED), RGB(0, 192, .0));//_T("Freed")
		
		if (l4 > 0) // freed
		{
			dPercent = ((double)l4 / (double) lt) * 100.00;
			pData->AddPie2 (VDBA_MfcResourceString(IDS_I_NEVER_USED), dPercent, RGB(255, 255, 0), (double)l4);//_T("Never Used")
		}
		else
			pData->AddLegend (VDBA_MfcResourceString(IDS_I_NEVER_USED), RGB(255, 255, 0));//_T("Never Used")
		
		
		strTitle.Format (VDBA_MfcResourceString(IDS_F_TOTAL), (l1+l2+l3+l4));//_T("Total = %d")
		pData->SetTitle(strTitle);
		m_pPieCtrl->UpdateLegend();
		m_pPieCtrl->UpdatePieChart();
	}
	UpdateData (FALSE);
}

/////////////////////////////////
// Cells Legends management

void CuDlgDomPropTblPages::UpdateCellLegends(BOOL bHashed)
{
  if (bHashed)
    SetCellLegendsToHashed();
  else
    SetCellLegendsToNonHashed();
}


void CuDlgDomPropTblPages::SetCellLegendsToNonHashed()
{
  m_stCell1.SetItemType(PAGE_HEADER);         // Free Header page
  m_stCell2.SetItemType(PAGE_MAP);            // Free Map    page

  m_stCell3.SetItemType(PAGE_FREE);           // Free page, has been used and subsequently made free
  m_stPg3.SetWindowText(VDBA_MfcResourceString(IDS_I_FREE));//_T("Free")

  m_stCell4.SetItemType(PAGE_UNUSED);         // Unused page, has never been written to
  m_stCell4.ShowWindow(SW_SHOW);
  m_stPg4.ShowWindow(SW_SHOW);

  m_stCell5.SetItemType(PAGE_DATA);           // Data page
  m_stPg5.SetWindowText(VDBA_MfcResourceString(IDS_I_DATA));//_T("Data")
  m_stCell6.SetItemType(PAGE_ASSOCIATED);     // Associated data page
  m_stPg6.SetWindowText(VDBA_MfcResourceString(IDS_I_ASSO_DATA));//_T("Associated data")
  m_stCell7.SetItemType(PAGE_OVERFLOW_LEAF);  // Overflow leaf page
  m_stPg7.SetWindowText(VDBA_MfcResourceString(IDS_I_OVERFLOW_LEAF));//_T("Overflow leaf")
  m_stCell8.SetItemType(PAGE_OVERFLOW_DATA);  // Overflow data page
  m_stPg8.SetWindowText(VDBA_MfcResourceString(IDS_I_OVERFLOW_DATA));//_T("Overflow data")

  m_stCell9.SetItemType(PAGE_ROOT);           // Root page
  m_stPg9.SetWindowText(VDBA_MfcResourceString(IDS_I_ROOT));//_T("Root")
  m_stCell10.SetItemType(PAGE_INDEX);         // Index page
  m_stPg10.SetWindowText(VDBA_MfcResourceString(IDS_I_PAGE_INDEX));//_T("Index")

  m_stCell11.ShowWindow(SW_SHOW);
  m_stPg11.ShowWindow(SW_SHOW);
  m_stCell12.ShowWindow(SW_SHOW);
  m_stPg12.ShowWindow(SW_SHOW);

  m_stCell11.SetItemType(PAGE_SPRIG);         // Sprig page
  m_stCell12.SetItemType(PAGE_LEAF);          // Leaf page
}

void CuDlgDomPropTblPages::SetCellLegendsToHashed()
{
  m_stCell1.SetItemType(PAGE_HEADER);         // Free Header page
  m_stCell2.SetItemType(PAGE_MAP);            // Free Map    page

  m_stCell3.SetItemType(PAGE_UNUSED);         // Unused page
  m_stPg3.SetWindowText(VDBA_MfcResourceString(IDS_I_UNUSED));//_T("Unused")

  m_stCell4.ShowWindow(SW_HIDE);
  m_stPg4.ShowWindow(SW_HIDE);

  m_stCell5.SetItemType(PAGE_DATA);                 // Data page
  m_stPg5.SetWindowText(VDBA_MfcResourceString(IDS_I_DATA_WO));//_T("Data w/o overflow"
  m_stCell6.SetItemType(PAGE_EMPTY_DATA_NO_OVF);    // Empty data, with no overflow page
  m_stPg6.SetWindowText(VDBA_MfcResourceString(IDS_I_EMPTY_WO));//_T("Empty w/o overflow")
  m_stCell7.SetItemType(PAGE_DATA_WITH_OVF);        // Non-Empty data, with overflow pages
  m_stPg7.SetWindowText(VDBA_MfcResourceString(IDS_I_DATA_WITH));//_T("Data with overflow")
  m_stCell8.SetItemType(PAGE_EMPTY_DATA_WITH_OVF);  // Empty data, wih overflow pages
  m_stPg8.SetWindowText(VDBA_MfcResourceString(IDS_I_EMPTY_WITH));//_T("Empty with overflow")

  m_stCell9.SetItemType(PAGE_OVERFLOW_DATA);        // Overflow data page
  m_stPg9.SetWindowText(VDBA_MfcResourceString(IDS_I_OVERFLOW_WITH_DTA));//_T("Overflow with data")
  m_stCell10.SetItemType(PAGE_EMPTY_OVERFLOW);      // Empty overflow page
  m_stPg10.SetWindowText(VDBA_MfcResourceString(IDS_I_EMPTY_OVERFLOW));//_T("Empty Overflow")

  m_stCell11.ShowWindow(SW_HIDE);
  m_stPg11.ShowWindow(SW_HIDE);
  m_stCell12.ShowWindow(SW_HIDE);
  m_stPg12.ShowWindow(SW_HIDE);
}


void CuDlgDomPropTblPages::OnPaletteChanged(CWnd* pFocusWnd) 
{
	CDialog::OnPaletteChanged(pFocusWnd);

	if (pFocusWnd != this)
		OnQueryNewPalette();
}

BOOL CuDlgDomPropTblPages::OnQueryNewPalette() 
{
	if (m_pPieCtrl && IsWindow(m_pPieCtrl->m_hWnd))
		m_pPieCtrl->Invalidate();
	CWnd* pBarWnd = m_pPieCtrlHashTable->GetCurrentView();

	if ((HPALETTE)m_DetailedPalette != NULL)
	{
		CWnd* array[24] = 
		{
			&m_cListCtrl,
			&m_st100Pct,
			&m_stCell1,
			&m_stCell2,
			&m_stCell3,
			&m_stCell4,
			&m_stCell5,
			&m_stCell6,
			&m_stCell7,
			&m_stCell8,
			&m_stCell9,
			&m_stCell10,
			&m_stCell11,
			&m_stCell12,

			&m_crI1,
			&m_crI2,
			&m_crI3,
			&m_crI4,
			&m_crI5,
			&m_crI6,
			&m_crI7,
			&m_crI8,
			&m_crI9,
			pBarWnd
		};

		UINT u = 0;
		int i;
		for (i=0; i<24; i++)
		{
			if (array[i] && IsWindow(array[i]->m_hWnd))
			{
				CClientDC   dc(array[i]);
				CPalette* pOldPal = dc.SelectPalette(&m_DetailedPalette, FALSE);
				u = dc.RealizePalette();
				dc.SelectPalette(pOldPal, FALSE);
			}
		}
		for (i=0; i<24; i++)
		{
			if (array[i] && IsWindow(array[i]->m_hWnd))
			{
				array[i]->Invalidate();
			}
		}
	}
	return CDialog::OnQueryNewPalette();
}

//
// bIn = TRUE  -> copy pPageinfo to pMemer
// bIn = FALSE -> copy pMemer to pPageinfo;
void CuDlgDomPropTblPages::CopyData(CuDomPropDataPages* pMemer, INGRESPAGEINFO* pPageInfo, BOOL bIn)
{
	if (bIn)
	{
		pMemer->m_lTotal        = pPageInfo->ltotal    ;
		pMemer->m_lInUse        = pPageInfo->linuse    ;
		pMemer->m_lOverflow     = pPageInfo->loverflow ;
		pMemer->m_lFreed        = pPageInfo->lfreed    ;
		pMemer->m_lNeverUsed    = pPageInfo->lneverused;

		pMemer->m_lItemsPerByte = pPageInfo->lItemsPerByte;
		pMemer->m_nbElements    = pPageInfo->nbElements;
		pMemer->m_pByteArray    = pPageInfo->pByteArray;

		pMemer->m_lbuckets_no_ovf = pPageInfo->lbuckets_no_ovf;
		pMemer->m_lemptybuckets_no_ovf = pPageInfo->lemptybuckets_no_ovf;
		pMemer->m_lbuck_with_ovf = pPageInfo->lbuck_with_ovf;
		pMemer->m_lemptybuckets_with_ovf = pPageInfo->lemptybuckets_with_ovf;
		pMemer->m_loverflow_not_empty = pPageInfo->loverflow_not_empty;
		pMemer->m_loverflow_empty = pPageInfo->loverflow_empty;

		pMemer->m_llongest_ovf_chain    = pPageInfo->llongest_ovf_chain;
		pMemer->m_lbuck_longestovfchain = pPageInfo->lbuck_longestovfchain;
		pMemer->m_favg_ovf_chain        = pPageInfo->favg_ovf_chain;

		// Set current number of pages per cell to most detailed possible value
		pMemer->m_lCurPgPerCell = pPageInfo->lItemsPerByte;
		pMemer->m_bHashed       = pPageInfo->bNewHash;
	}
	else
	{
		memset (pPageInfo, 0, sizeof(INGRESPAGEINFO));
		pPageInfo->ltotal = pMemer->m_lTotal;
		pPageInfo->linuse = pMemer->m_lInUse;
		pPageInfo->loverflow  = pMemer->m_lOverflow;
		pPageInfo->lfreed = pMemer->m_lFreed;
		pPageInfo->lneverused = pMemer->m_lNeverUsed;

		pPageInfo->lItemsPerByte = pMemer->m_lItemsPerByte;
		pPageInfo->nbElements = pMemer->m_nbElements;
		pPageInfo->pByteArray = pMemer->m_pByteArray;

		pPageInfo->lbuckets_no_ovf = pMemer->m_lbuckets_no_ovf;
		pPageInfo->lemptybuckets_no_ovf = pMemer->m_lemptybuckets_no_ovf;
		pPageInfo->lbuck_with_ovf = pMemer->m_lbuck_with_ovf;
		pPageInfo->lemptybuckets_with_ovf = pMemer->m_lemptybuckets_with_ovf;
		pPageInfo->loverflow_not_empty = pMemer->m_loverflow_not_empty;
		pPageInfo->loverflow_empty = pMemer->m_loverflow_empty;

		pPageInfo->llongest_ovf_chain    = pMemer->m_llongest_ovf_chain   ;
		pPageInfo->lbuck_longestovfchain = pMemer->m_lbuck_longestovfchain;
		pPageInfo->favg_ovf_chain        = pMemer->m_favg_ovf_chain       ;

		// Set current number of pages per cell to most detailed possible value
		pPageInfo->lItemsPerByte =   pMemer->m_lCurPgPerCell;
		pPageInfo->bNewHash      =  pMemer->m_bHashed ;
	}
}
