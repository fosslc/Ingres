/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpDb.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_E_NO_EXTEND_LOC
**	    into two statements to avoid compiler error.
**  21-Jun-2001 (schph01)
**      (SIR 103694) STEP 2 support of UNICODE datatypes
**  15-May-2003 (schph01)
**      bug 110247 remove the code to retrieve the dbmsinfo('unicode_level')
**      and transfers in GetDetailInfo() function (mainlib\dbagdet.c)
**  10-Jun-2004 (schph01)
**      bug 112460 Manage "Catalogs Page Size" information
**  06-Sep-2005) (drivi01)
**      Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**      two available unicode normalization options, added group box with
**      two checkboxes corresponding to each normalization, including all
**      expected functionlity.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpdb.h"

#include "localter.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "dll.h"
  #include "dlgres.h"
  #include "dbaginfo.h"
  extern LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static CTLDATA prodTypes [] =
{
  PROD_INGRES,        IDS_PROD_INGRES,
  PROD_VISION,        IDS_PROD_VISION,
  PROD_W4GL,          IDS_PROD_W4GL,
  PROD_INGRES_DBD,    IDS_PROD_INGRES_DBD,
  // PROD_NOFECLIENTS,   IDS_PROD_NOFECLIENTS,
  -1// Terminator
};

#define ALTERNATE_NUMBER  4
#define LOCEXT_NUMBER     3
static int aGrantType[] = { LOCEXT_DATABASE,
                            LOCEXT_WORK,
                            LOCEXT_AUXILIARY
};

IMPLEMENT_DYNCREATE(CuDlgDomPropDb, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDb dialog


CuDlgDomPropDb::CuDlgDomPropDb()
  : CFormView(CuDlgDomPropDb::IDD)
{
  //{{AFX_DATA_INIT(CuDlgDomPropDb)
  //}}AFX_DATA_INIT
  m_bNeedSubclass = TRUE;
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_DATABASE);
}

CuDlgDomPropDb::~CuDlgDomPropDb()
{
}

void CuDlgDomPropDb::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropDb)
	DDX_Control(pDX, IDC_STATIC_UNICODE, m_ctrlStaticUnicode);
	DDX_Control(pDX, IDC_STATIC_UNICODE2, m_ctrlStaticUnicode2);
	DDX_Control(pDX, IDC_CHECK_UNICODE, m_ctrlUnicode);
	DDX_Control(pDX, IDC_CHECK_UNICODE2, m_ctrlUnicode2);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCCKP, m_edLocChk);
	DDX_Control(pDX, IDC_DOMPROP_DB_OWNER, m_edOwner);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCSORT, m_edLocSrt);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCJNL, m_edLocJnl);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCDMP, m_edLocDmp);
	DDX_Control(pDX, IDC_DOMPROP_DB_LOCDB, m_edLocDb);
	DDX_Control(pDX, IDC_READONLY, m_ReadOnly);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_STATIC_CAT_PAGE_SIZE, m_StatCtrlCatPageSize);
	DDX_Control(pDX, IDC_EDIT_CAT_PAGE_SIZE, m_ctrlEditCatPageSize);
}

void CuDlgDomPropDb::AddAltLoc (CuAltLoc* pAltLoc)
{
  ASSERT (pAltLoc);
  BOOL  bSpecial = pAltLoc->IsSpecialItem();

  int nCount, index;
  nCount = m_cListAlternateLocs.GetItemCount ();

  // Name
  index  = m_cListAlternateLocs.InsertItem (nCount, (LPCTSTR)pAltLoc->GetStrName(), 0);

  // columns
  for (int cpt = 0; cpt < LOCEXT_NUMBER; cpt++) {
    if (bSpecial)
      m_cListAlternateLocs.SetItemText (index, cpt+1, "");
    else {
      if (pAltLoc->GetFlag(aGrantType[cpt]))
        m_cListAlternateLocs.SetCheck (index, cpt+1, TRUE);
      else
        m_cListAlternateLocs.SetCheck (index, cpt+1, FALSE);
    }
  }

  // Attach pointer on Location, for future sort purposes
  m_cListAlternateLocs.SetItemData(index, (DWORD)pAltLoc);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropDb, CFormView)
  //{{AFX_MSG_MAP(CuDlgDomPropDb)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDb diagnostics

#ifdef _DEBUG
void CuDlgDomPropDb::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropDb::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDb message handlers

void CuDlgDomPropDb::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropDb::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropDb::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListAlternateLocs);
        return 0L;
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
  DATABASEPARAMS DbParams;
  memset (&DbParams, 0, sizeof (DbParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get Detail Info
  //
  lstrcpy ((LPTSTR)DbParams.objectname, (LPCTSTR)lpRecord->objName);
  DbParams.DbType = lpRecord->parentDbType;
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_DATABASE,
                               &DbParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataDb tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csOwner  = DbParams.szOwner ;
  m_Data.m_csLocDb  = DbParams.szDbDev ;
  m_Data.m_csLocChk = DbParams.szChkpDev;
  m_Data.m_csLocJnl = DbParams.szJrnlDev;
  m_Data.m_csLocDmp = DbParams.szDmpDev;
  m_Data.m_csLocSrt = DbParams.szSortDev;
  m_Data.m_bReadOnly = DbParams.bReadOnly;

  // New fields
  m_Data.m_bPrivate     = DbParams.bPrivate;
  m_Data.m_csFeCatalogs = DbParams.szCatalogsOIToolsAvailables;

  // New array of Alternate locations
  m_Data.m_uaAltLocs.RemoveAll();
  LPOBJECTLIST lplist;
  for (lplist = DbParams.lpDBExtensions; lplist; lplist = (LPOBJECTLIST)lplist->lpNext) {
    LPDB_EXTENSIONS lpObjTemp = (LPDB_EXTENSIONS)lplist->lpObject;
    ASSERT (lpObjTemp);
    int status = lpObjTemp->status;
    ASSERT (status == LOCEXT_DATABASE || status == LOCEXT_WORK || status == LOCEXT_AUXILIARY);
    CuAltLoc altLoc((const char *)lpObjTemp->lname,  // Location name
                           FALSE,                     // not special item
                           status                     // location usage
                           );
    CuMultFlag *pRefAltLoc = m_Data.m_uaAltLocs.Find(&altLoc);
    if (pRefAltLoc)
      m_Data.m_uaAltLocs.Merge(pRefAltLoc, &altLoc);
    else
      m_Data.m_uaAltLocs.Add(altLoc);
  }
  if (m_Data.m_uaAltLocs.GetCount() == 0)     // Manage no alternate location
  {
      /* "<No Extend location>" */
      CuAltLoc altLoc2(VDBA_MfcResourceString (IDS_E_NO_EXTEND_LOC), TRUE);
      m_Data.m_uaAltLocs.Add(altLoc2);
  }
  ASSERT (m_Data.m_uaAltLocs.GetCount() > 0 );

  m_Data.m_bUnicodeDB = 0;
  if (DbParams.bUnicodeDBNFD)
	m_Data.m_bUnicodeDB = 1;
  if (DbParams.bUnicodeDBNFC)
	m_Data.m_bUnicodeDB = 2;

  m_Data.m_csCatalogsPageSize = DbParams.szCatalogsPageSize;

  // liberate detail structure
  FreeAttachedPointers (&DbParams, OT_DATABASE);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDb::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataDb") == 0);

  // ensure controls subclassed
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }

  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataDb* pData = (CuDomPropDataDb*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataDb)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDb::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataDb* pData = new CuDomPropDataDb(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropDb::RefreshDisplay()
{
  // Must have already been subclassed by OnInitialUpdate()
  ASSERT (!m_bNeedSubclass);
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }

  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  m_edOwner.SetWindowText(m_Data.m_csOwner);
  m_edLocDb.SetWindowText(m_Data.m_csLocDb);
  m_edLocChk.SetWindowText(m_Data.m_csLocChk);
  m_edLocJnl.SetWindowText(m_Data.m_csLocJnl);
  m_edLocDmp.SetWindowText(m_Data.m_csLocDmp);
  m_edLocSrt.SetWindowText(m_Data.m_csLocSrt);
  m_ReadOnly.SetCheck(m_Data.m_bReadOnly ? 1 : 0);

  if (GetOIVers() >= OIVERS_30)
  {
    
    m_StatCtrlCatPageSize.ShowWindow(SW_SHOW);
    m_ctrlEditCatPageSize.ShowWindow(SW_SHOW);
    m_ctrlEditCatPageSize.SetWindowText(m_Data.m_csCatalogsPageSize);
  }
  else
  {
    m_StatCtrlCatPageSize.ShowWindow(SW_HIDE);
    m_ctrlEditCatPageSize.ShowWindow(SW_HIDE);
  }
  if (GetOIVers() >= OIVERS_26)
  {
    m_ctrlUnicode.ShowWindow(SW_SHOW);
    m_ctrlUnicode2.ShowWindow(SW_SHOW);
    m_ctrlStaticUnicode.ShowWindow(SW_SHOW);
    m_ctrlStaticUnicode2.ShowWindow(SW_SHOW);
    m_ctrlUnicode.SetCheck((m_Data.m_bUnicodeDB == 1) ? 1 : 0);
    m_ctrlUnicode2.SetCheck((m_Data.m_bUnicodeDB == 2) ? 1 : 0);
  }
  else
  {
    m_ctrlUnicode.ShowWindow(SW_HIDE);
    m_ctrlUnicode2.ShowWindow(SW_HIDE);
    m_ctrlStaticUnicode.ShowWindow(SW_HIDE);
    m_ctrlStaticUnicode2.ShowWindow(SW_HIDE);
  }

  // private/public
  CheckRadioButton( IDC_PUBLIC, IDC_PRIVATE, m_Data.m_bPrivate? IDC_PRIVATE: IDC_PUBLIC);

  // Installed front end catalogs
  int nCount = m_CheckFrontEndList.GetCount();
  for (int i = 0; i < nCount; i++) {
    m_CheckFrontEndList.EnableItem(i,FALSE);        // always disabled
    m_CheckFrontEndList.SetCheck(i, FALSE);         // reset by default
    DWORD dwtype = m_CheckFrontEndList.GetItemData(i);
    if ( (dwtype == PROD_INGRES     && m_Data.m_csFeCatalogs[TOOLS_INGRES]    == 'Y')
      || (dwtype == PROD_VISION     && m_Data.m_csFeCatalogs[TOOLS_VISION]    == 'Y')
      || (dwtype == PROD_W4GL       && m_Data.m_csFeCatalogs[TOOLS_W4GL]      == 'Y')
      || (dwtype == PROD_INGRES_DBD && m_Data.m_csFeCatalogs[TOOLS_INGRESDBD] == 'Y')
      )
      m_CheckFrontEndList.SetCheck(i);
  }

  // alternate locations
  m_cListAlternateLocs.DeleteAllItems();
  int size = m_Data.m_uaAltLocs.GetCount();
  ASSERT (size > 0);
  for (int cnt = 0; cnt < size; cnt++) {
    CuAltLoc* pAltLoc = m_Data.m_uaAltLocs[cnt];
    AddAltLoc(pAltLoc);
  }
  // Initial sort on names
  m_cListAlternateLocs.SortItems(CuAltLoc::CompareNames, 0L);
}

void CuDlgDomPropDb::SubclassControls()
{
  // list of installed Front End Catalogs
  VERIFY (m_CheckFrontEndList.SubclassDlgItem (IDC_LIST1, this));
  LPCTLDATA lpdata = prodTypes;
  // m_CheckFrontEndList.ResetContent();
  for (int index=0; lpdata[index].nId != -1; index++) {
    CString Tempo;
    int nIdx;
    Tempo.LoadString(lpdata[index].nStringId);
    nIdx = m_CheckFrontEndList.AddString(Tempo);
    if (nIdx != LB_ERR)
      m_CheckFrontEndList.SetItemData(nIdx, lpdata[index].nId);
  }
  ASSERT (m_CheckFrontEndList.GetCount() > 0);

  // Alternate locations list control
  VERIFY (m_cListAlternateLocs.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(1, 18, TRUE, 1, 0);
  m_cListAlternateLocs.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListAlternateLocs.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aAlternateColumns[ALTERNATE_NUMBER] = {
    { _T(""),          FALSE, 160 },
    { _T("Database"),               TRUE,  -1  },
    { _T("Work"),                   TRUE,  -1  },
    { _T("Auxiliary Work"),         TRUE,  -1  },
  };
  lstrcpy(aAlternateColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_LOC_NAME));
  InitializeColumns(m_cListAlternateLocs, aAlternateColumns, ALTERNATE_NUMBER);
}

void CuDlgDomPropDb::ResetDisplay()
{
  ASSERT (!m_bNeedSubclass);

  UpdateData (FALSE);   // Mandatory!

  m_edOwner.SetWindowText(_T(""));
  m_edLocDb.SetWindowText(_T(""));
  m_edLocChk.SetWindowText(_T(""));
  m_edLocJnl.SetWindowText(_T(""));
  m_edLocDmp.SetWindowText(_T(""));
  m_edLocSrt.SetWindowText(_T(""));
  m_ReadOnly.SetCheck(0);

  if (GetOIVers() >= OIVERS_30)
  {
    
    m_StatCtrlCatPageSize.ShowWindow(SW_SHOW);
    m_ctrlEditCatPageSize.ShowWindow(SW_SHOW);
    m_ctrlEditCatPageSize.SetWindowText(_T(""));
  }
  else
  {
    m_StatCtrlCatPageSize.ShowWindow(SW_HIDE);
    m_ctrlEditCatPageSize.ShowWindow(SW_HIDE);
  }

  if (GetOIVers() >= OIVERS_26)
  {
    m_ctrlUnicode.ShowWindow(SW_SHOW);
    m_ctrlUnicode2.ShowWindow(SW_SHOW);
    m_ctrlStaticUnicode.ShowWindow(SW_SHOW);
    m_ctrlStaticUnicode2.ShowWindow(SW_SHOW);
    m_ctrlUnicode.SetCheck(0);
    m_ctrlUnicode2.SetCheck(0);
  }
  else
  {
    m_ctrlUnicode.ShowWindow(SW_HIDE);
    m_ctrlUnicode2.ShowWindow(SW_HIDE);
    m_ctrlStaticUnicode.ShowWindow(SW_HIDE);
    m_ctrlStaticUnicode2.ShowWindow(SW_HIDE);
  }

  // private/public
  CheckRadioButton( IDC_PUBLIC, IDC_PRIVATE, IDC_PUBLIC);

  // Installed front end catalogs
  int nCount = m_CheckFrontEndList.GetCount();
  for (int i = 0; i < nCount; i++) {
    m_CheckFrontEndList.EnableItem(i,FALSE);        // always disabled
    m_CheckFrontEndList.SetCheck(i, FALSE);         // reset by default
  }

  // alternate locations
  m_cListAlternateLocs.DeleteAllItems();

  VDBA_OnGeneralSettingChange(&m_cListAlternateLocs);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

void CuDlgDomPropDb::OnInitialUpdate() 
{
  CFormView::OnInitialUpdate();
  
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }
  
}
