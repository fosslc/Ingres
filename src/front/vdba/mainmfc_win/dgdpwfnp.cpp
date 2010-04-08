// DgDpWFNP.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpwfnp.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  //#include "monitor.h"
  #include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgDomPropIceFacetAndPage, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceFacetAndPage dialog


CuDlgDomPropIceFacetAndPage::CuDlgDomPropIceFacetAndPage()
	: CFormView(CuDlgDomPropIceFacetAndPage::IDD)
{
	//{{AFX_DATA_INIT(CuDlgDomPropIceFacetAndPage)
	//}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ICE_GENERIC);
}

CuDlgDomPropIceFacetAndPage::~CuDlgDomPropIceFacetAndPage()
{
}

void CuDlgDomPropIceFacetAndPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropIceFacetAndPage)
	DDX_Control(pDX, IDC_ICE_ST_FILENAME, m_stFilename);
	DDX_Control(pDX, IDC_ICE_ST_LOC, m_stLocation);
	DDX_Control(pDX, IDC_ST_PATH, m_stPath);
	DDX_Control(pDX, IDC_ST_SESSION_CACHE, m_stSessionCache);
	DDX_Control(pDX, IDC_ST_PRE_CACHE, m_stPreCache);
	DDX_Control(pDX, IDC_ST_PERM_CACHE, m_stPermCache);
	DDX_Control(pDX, IDC_PATH, m_edPath);
	DDX_Control(pDX, IDC_ICE_STATIC_CACHE, m_stGroupCache);
	DDX_Control(pDX, IDC_ICE_LOCATION, m_edLocation);
	DDX_Control(pDX, IDC_ICE_FILENAMEPLUSEXT, m_edFilename);
	DDX_Control(pDX, IDC_ICE_DOC_NAMEPLUSEXT, m_edDoc);
	DDX_Control(pDX, IDC_ICE_ACCESS, m_chkPublic);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceFacetAndPage, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropIceFacetAndPage)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceFacetAndPage diagnostics

#ifdef _DEBUG
void CuDlgDomPropIceFacetAndPage::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropIceFacetAndPage::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIceFacetAndPage message handlers

void CuDlgDomPropIceFacetAndPage::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropIceFacetAndPage::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropIceFacetAndPage::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  ResetDisplay();
  // Get info on the object
  ICEBUSUNITDOCDATA IceFacetAndPageParams;
  memset (&IceFacetAndPageParams, 0, sizeof (IceFacetAndPageParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get ICE Detail Info
  //
  int objType = lpRecord->recType;
  ASSERT (objType == OT_ICE_BUNIT_FACET || objType == OT_ICE_BUNIT_PAGE);
  m_Data.m_objType = objType;   // MANDATORY!

  // prepare parenthood: name of business unit
  lstrcpy ((LPTSTR)IceFacetAndPageParams.icebusunit.Name, (LPCTSTR)lpRecord->extra);

  CString csName = (LPCTSTR)lpRecord->objName;
  int separator = csName.Find(_T('.'));
  ASSERT (separator != -1);     // FNN VA ENVOYER NAME.SUFFIX - PAS ENCORE PRÊT!
  ASSERT (separator > 0);
  if (separator > 0) {
    lstrcpy ((LPTSTR)IceFacetAndPageParams.name, (LPCTSTR)csName.Left(separator));
    lstrcpy ((LPTSTR)IceFacetAndPageParams.suffix, (LPCTSTR)csName.Mid(separator+1));
  }
  else {
    lstrcpy ((LPTSTR)IceFacetAndPageParams.name, (LPCTSTR)lpRecord->objName);
    IceFacetAndPageParams.suffix[0] = '\0';
  }

  int iResult = GetICEInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                            objType,
                            &IceFacetAndPageParams);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataIceFacetAndPage tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  //
  // update member variables, for display/load/save purpose
  //

  // Common info
  m_Data.m_csNamePlusExt = IceFacetAndPageParams.name;
  m_Data.m_csNamePlusExt += _T('.');
  m_Data.m_csNamePlusExt += IceFacetAndPageParams.suffix;
  m_Data.m_bPublic = IceFacetAndPageParams.bPublic;

  // Storage type - criterion has changed!
  if (IceFacetAndPageParams.bpre_cache ||
	  IceFacetAndPageParams.bperm_cache ||
	  IceFacetAndPageParams.bsession_cache)
    m_Data.m_bStorageInsideRepository = TRUE;
  else
    m_Data.m_bStorageInsideRepository = FALSE;

  // specific for Inside repository
  if (m_Data.m_bStorageInsideRepository) {
    m_Data.m_bPreCache = m_Data.m_bPermCache = m_Data.m_bSessionCache = FALSE;

    if (IceFacetAndPageParams.bpre_cache)
      m_Data.m_bPreCache = TRUE;
    else if (IceFacetAndPageParams.bperm_cache)
      m_Data.m_bPermCache = TRUE;
    else if (IceFacetAndPageParams.bsession_cache)
      m_Data.m_bSessionCache = TRUE;

    m_Data.m_csPath = IceFacetAndPageParams.doc_file;
  }

  // specific for File system
  if (!m_Data.m_bStorageInsideRepository) {
    m_Data.m_csLocation = IceFacetAndPageParams.ext_loc.LocationName;

    m_Data.m_csFilename = IceFacetAndPageParams.ext_file;
    m_Data.m_csFilename += _T('.');
    m_Data.m_csFilename += IceFacetAndPageParams.ext_suffix;
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceFacetAndPage::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceFacetAndPage") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceFacetAndPage* pData = (CuDomPropDataIceFacetAndPage*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceFacetAndPage)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceFacetAndPage::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceFacetAndPage* pData = new CuDomPropDataIceFacetAndPage(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropIceFacetAndPage::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
  ShowHideControls();

  // common info
  m_edDoc.SetWindowText(m_Data.m_csNamePlusExt);
  m_chkPublic.SetCheck(m_Data.m_bPublic);

  // Storage type
  if (m_Data.m_bStorageInsideRepository)
    CheckRadioButton(IDC_REPOSITORY, IDC_FILE_SYSTEM, IDC_REPOSITORY);
  else
    CheckRadioButton(IDC_REPOSITORY, IDC_FILE_SYSTEM, IDC_FILE_SYSTEM);

  // specific for Inside repository
  if (m_Data.m_bStorageInsideRepository) {

    if (m_Data.m_bPreCache)
      CheckRadioButton (IDC_PRE_CACHE, IDC_SESSION_CACHE, IDC_PRE_CACHE);
    else if (m_Data.m_bPermCache)
      CheckRadioButton (IDC_PRE_CACHE, IDC_SESSION_CACHE, IDC_PERM_CACHE);
    else if ((m_Data.m_bSessionCache))
      CheckRadioButton (IDC_PRE_CACHE, IDC_SESSION_CACHE, IDC_SESSION_CACHE);
    else
      CheckRadioButton (IDC_PRE_CACHE, IDC_SESSION_CACHE, IDC_PRE_CACHE);

    m_edPath.SetWindowText(m_Data.m_csPath);
  }

  // specific for File system
  if (!m_Data.m_bStorageInsideRepository) {
    m_edLocation.SetWindowText(m_Data.m_csLocation);
    m_edFilename.SetWindowText(m_Data.m_csFilename);
  }
}

void CuDlgDomPropIceFacetAndPage::ShowHideControls()
{
  BOOL bShowInside = FALSE;
  BOOL bShowFilesys = FALSE;

  if (m_Data.m_bStorageInsideRepository)
    bShowInside = TRUE;
  else
    bShowFilesys = TRUE;

  // Related to repository:
  m_stGroupCache.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  m_stPreCache.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  m_stPermCache.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  m_stSessionCache.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  GetDlgItem(IDC_PRE_CACHE)->ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  GetDlgItem(IDC_PERM_CACHE)->ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  GetDlgItem(IDC_SESSION_CACHE)->ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  m_stPath.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);
  m_edPath.ShowWindow(bShowInside? SW_SHOW: SW_HIDE);

  // Related to file system
  m_stLocation.ShowWindow(bShowFilesys? SW_SHOW: SW_HIDE);
  m_edLocation.ShowWindow(bShowFilesys? SW_SHOW: SW_HIDE);
  m_stFilename.ShowWindow(bShowFilesys? SW_SHOW: SW_HIDE);
  m_edFilename.ShowWindow(bShowFilesys? SW_SHOW: SW_HIDE);
}

void CuDlgDomPropIceFacetAndPage::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_edDoc.SetWindowText(_T(""));
  m_chkPublic.SetCheck(0);
  m_edPath.SetWindowText(_T(""));
  m_edLocation.SetWindowText(_T(""));
  m_edFilename.SetWindowText(_T(""));
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}