/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpGrXD.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_GRANTEE into two statements to avoid compiler error.
**  09-Apr-2003 (schph01)
**     SIR 107523 Add columns for Create Sequence Grantee
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpgrxd.h"
#include "vtree.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int aGrantedType[NBDBGRANTED] = {
                                        OTR_DBGRANT_ACCESY_DB,
                                        OTR_DBGRANT_ACCESN_DB,
                                        OTR_DBGRANT_CREPRY_DB,
                                        OTR_DBGRANT_CREPRN_DB,
                                        OTR_DBGRANT_CRETBY_DB,
                                        OTR_DBGRANT_CRETBN_DB,
                                        OTR_DBGRANT_DBADMY_DB,
                                        OTR_DBGRANT_DBADMN_DB,
                                        OTR_DBGRANT_LKMODY_DB,
                                        OTR_DBGRANT_LKMODN_DB,
                                        OTR_DBGRANT_QRYIOY_DB,
                                        OTR_DBGRANT_QRYION_DB,
                                        OTR_DBGRANT_QRYRWY_DB,
                                        OTR_DBGRANT_QRYRWN_DB,
                                        OTR_DBGRANT_UPDSCY_DB,
                                        OTR_DBGRANT_UPDSCN_DB,
                                        OTR_DBGRANT_SELSCY_DB,
                                        OTR_DBGRANT_SELSCN_DB,
                                        OTR_DBGRANT_CNCTLY_DB,
                                        OTR_DBGRANT_CNCTLN_DB,
                                        OTR_DBGRANT_IDLTLY_DB,
                                        OTR_DBGRANT_IDLTLN_DB,
                                        OTR_DBGRANT_SESPRY_DB,
                                        OTR_DBGRANT_SESPRN_DB,
                                        OTR_DBGRANT_TBLSTY_DB,
                                        OTR_DBGRANT_TBLSTN_DB,
                                        OTR_DBGRANT_QRYCPY_DB,
                                        OTR_DBGRANT_QRYCPN_DB,
                                        OTR_DBGRANT_QRYPGY_DB,
                                        OTR_DBGRANT_QRYPGN_DB,
                                        OTR_DBGRANT_QRYCOY_DB,
                                        OTR_DBGRANT_QRYCON_DB,
                                        OTR_DBGRANT_SEQCRY_DB, /* these need to be the 2 last ones, since they are to*/
                                        OTR_DBGRANT_SEQCRN_DB, /* appear depending on the return value ofGetOIVers() */
                                        };
#define LAYOUT_NUMBER   ( NBDBGRANTED + 1 )

CuDlgDomPropGroupXGrantedDb::CuDlgDomPropGroupXGrantedDb(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropGroupXGrantedDb::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropGroupXGrantedDb)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropGroupXGrantedDb::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropGroupXGrantedDb)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropGroupXGrantedDb::AddGroupXGrantedDb (CuGrantedDb* pGrantedDb)
{
  ASSERT (pGrantedDb);
  BOOL  bSpecial = pGrantedDb->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name
  index  = m_cListCtrl.InsertItem (nCount, pGrantedDb->GetStrName(), 0);

  for (int cpt = 0; cpt < NBDBGRANTED; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, _T(""));
    else {
      if (pGrantedDb->GetFlag(aGrantedType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+1, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+1, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+1, FALSE);
    }
  }

  // Attach pointer on granted object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pGrantedDb);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropGroupXGrantedDb, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropGroupXGrantedDb)
    ON_WM_SIZE()
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropGroupXGrantedDb message handlers

void CuDlgDomPropGroupXGrantedDb::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropGroupXGrantedDb::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_DATABASE);
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T(""),           FALSE, 80 + 16+4 }, // 16+4 for image width
    {   _T("access"),    TRUE,  -1 },     //  OT_DBGRANT_ACCESY_USER,
    {   _T("no access"),    TRUE,  -1 },  //  OT_DBGRANT_ACCESN_USER,
    {   _T("crepr"),    TRUE,  -1 },      //  OT_DBGRANT_CREPRY_USER,
    {   _T("no crepr"),    TRUE,  -1 },   //  OT_DBGRANT_CREPRN_USER,
    {   _T("cretb"),    TRUE,  -1 },      //  OT_DBGRANT_CRETBY_USER,
    {   _T("no cretb"),    TRUE,  -1 },   //  OT_DBGRANT_CRETBN_USER,
    {   _T("dbadm"),    TRUE,  -1 },      //  OT_DBGRANT_DBADMY_USER,
    {   _T("no dbadm"),    TRUE,  -1 },   //  OT_DBGRANT_DBADMN_USER,
    {   _T("lkmod"),    TRUE,  -1 },      //  OT_DBGRANT_LKMODY_USER,
    {   _T("no lkmod"),    TRUE,  -1 },   //  OT_DBGRANT_LKMODN_USER,
    {   _T("qryio"),    TRUE,  -1 },      //  OT_DBGRANT_QRYIOY_USER,
    {   _T("no qryio"),    TRUE,  -1 },   //  OT_DBGRANT_QRYION_USER,
    {   _T("qryrw"),    TRUE,  -1 },      //  OT_DBGRANT_QRYRWY_USER,
    {   _T("no qryrw"),    TRUE,  -1 },   //  OT_DBGRANT_QRYRWN_USER,
    {   _T("updsc"),    TRUE,  -1 },      //  OT_DBGRANT_UPDSCY_USER,
    {   _T("no updsc"),    TRUE,  -1 },   //  OT_DBGRANT_UPDSCN_USER,

    {   _T("selsc"),    TRUE,  -1 },      //  OT_DBGRANT_SELSCY_USER,
    {   _T("no selsc"),    TRUE,  -1 },   //  OT_DBGRANT_SELSCN_USER,
    {   _T("cnctl"),    TRUE,  -1 },      //  OT_DBGRANT_CNCTLY_USER,
    {   _T("no cnctl"),    TRUE,  -1 },   //  OT_DBGRANT_CNCTLN_USER,
    {   _T("idltl"),    TRUE,  -1 },      //  OT_DBGRANT_IDLTLY_USER,
    {   _T("no idltl"),    TRUE,  -1 },   //  OT_DBGRANT_IDLTLN_USER,
    {   _T("sespr"),    TRUE,  -1 },      //  OT_DBGRANT_SESPRY_USER,
    {   _T("no sespr"),    TRUE,  -1 },   //  OT_DBGRANT_SESPRN_USER,
    {   _T("tblst"),    TRUE,  -1 },      //  OT_DBGRANT_TBLSTY_USER,
    {   _T("no tblst"),    TRUE,  -1 },   //  OT_DBGRANT_TBLSTN_USER,

    {   _T("qrycpu"),    TRUE,  -1 },        //  OT_DBGRANT_QRYCPY_USER,
    {   _T("no qrycpu"),    TRUE,  -1 },     //  OT_DBGRANT_QRYCPN_USER,
    {   _T("qrypage"),    TRUE,  -1 },       //  OT_DBGRANT_QRYPGY_USER,
    {   _T("no qrypage"),    TRUE,  -1 },    //  OT_DBGRANT_QRYPGN_USER,
    {   _T("qrycost"),    TRUE,  -1 },       //  OT_DBGRANT_QRYCOY_USER,
    {   _T("no qrycost"),    TRUE,  -1 },    //  OT_DBGRANT_QRYCON_USER,
    {   _T("sequence"),    TRUE,  -1 },      //  OT_DBGRANT_SEQCRY_USER,
    {   _T("no sequence"),    TRUE,  -1 }    //  OT_DBGRANT_SEQCRN_USER,
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  InitializeColumns(m_cListCtrl, aColumns, GetOIVers()>=OIVERS_30? LAYOUT_NUMBER: LAYOUT_NUMBER-2);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropGroupXGrantedDb::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}

LONG CuDlgDomPropGroupXGrantedDb::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropGroupXGrantedDb::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      case FILTER_DOM_SYSTEMOBJECTS:  // can be system objects (replicator dd_xyz)
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_DATABASE)
        return 0L;
      break;
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListCtrl);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  ResetDisplay();
  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get list of granted objects
  //
  m_Data.m_uaGroupXGrantedDb.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // Xref return code
  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBDBGRANTED; index++) {
    iret =  DOMGetFirstRelObject(nNodeHandle,
                                 aGrantedType[index],
                                 1,                           // level,
                                 aparentsTemp,                // Temp mandatory!
                                 pUps->pSFilter->bWithSystem, // bwithsystem
                                 NULL,                        // base owner
                                 NULL,                        // other owner
                                 aparentsResult,
                                 buf,
                                 bufOwner,
                                 bufComplim);
     if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
       bError = TRUE;
       continue;
    }
    else {
      while (iret == RES_SUCCESS) {
        CuGrantedDb grantedDb((const char *)buf,                // obj name
                              (const char *)_T(""),             // No obj owner
                              (const char *)_T(""),             // No obj parent
                              aGrantedType[index]               // grant type
                              );
        CuMultFlag *pRefGrantedDb = m_Data.m_uaGroupXGrantedDb.Find(&grantedDb);
        if (pRefGrantedDb)
          m_Data.m_uaGroupXGrantedDb.Merge(pRefGrantedDb, &grantedDb);
        else
          m_Data.m_uaGroupXGrantedDb.Add(grantedDb);

        iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuGrantedDb grantedDb1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaGroupXGrantedDb.Add(grantedDb1);
  }

  // Manage no granted object
  if (m_Data.m_uaGroupXGrantedDb.GetCount() == 0)
  {
    /* "<No GrantedDb>" */
    CuGrantedDb grantedDb2(VDBA_MfcResourceString (IDS_E_NO_GRANTED_DB));
    m_Data.m_uaGroupXGrantedDb.Add(grantedDb2);
  }

  ASSERT (m_Data.m_uaGroupXGrantedDb.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropGroupXGrantedDb::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataGroupXGrantedDb") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataGroupXGrantedDb* pData = (CuDomPropDataGroupXGrantedDb*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataGroupXGrantedDb)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropGroupXGrantedDb::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataGroupXGrantedDb* pData = new CuDomPropDataGroupXGrantedDb(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropGroupXGrantedDb::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaGroupXGrantedDb.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuGrantedDb* pGrantedDb = m_Data.m_uaGroupXGrantedDb[cnt];
    AddGroupXGrantedDb(pGrantedDb);
  }
  // No Initial sort
}

void CuDlgDomPropGroupXGrantedDb::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuGranted::CompareNames, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 1);
      break;
  }
  *pResult = 0;
}


void CuDlgDomPropGroupXGrantedDb::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
