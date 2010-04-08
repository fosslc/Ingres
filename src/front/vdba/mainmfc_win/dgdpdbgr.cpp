/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpDbGr.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_GRANTEE into two statements to avoid compiler error.
**  10-Dec-2001 (noifr01)
**      (sir 99596) removal of obsolete code and resources
**  01-Apr-2003 (noifr01)
**   (sir 107523) management of sequences
**  23-Jan-2004 (schph01)
**     (sir 104378) detect version 3 of Ingres, for managing
**     new features provided in this version. replaced references
**     to 2.65 with refereces to 3  in #define definitions for
**     better readability in the future
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpdbgr.h"
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

static int aGrantType[NBDBGRANTEES] = {
                                       OT_DBGRANT_ACCESY_USER,
                                       OT_DBGRANT_ACCESN_USER,
                                       OT_DBGRANT_CREPRY_USER,
                                       OT_DBGRANT_CREPRN_USER,
                                       OT_DBGRANT_CRETBY_USER,
                                       OT_DBGRANT_CRETBN_USER,
                                       OT_DBGRANT_DBADMY_USER,
                                       OT_DBGRANT_DBADMN_USER,
                                       OT_DBGRANT_LKMODY_USER,
                                       OT_DBGRANT_LKMODN_USER,
                                       OT_DBGRANT_QRYIOY_USER,
                                       OT_DBGRANT_QRYION_USER,
                                       OT_DBGRANT_QRYRWY_USER,
                                       OT_DBGRANT_QRYRWN_USER,
                                       OT_DBGRANT_UPDSCY_USER,
                                       OT_DBGRANT_UPDSCN_USER,
                                       
                                       OT_DBGRANT_SELSCY_USER,
                                       OT_DBGRANT_SELSCN_USER,
                                       OT_DBGRANT_CNCTLY_USER,
                                       OT_DBGRANT_CNCTLN_USER,
                                       OT_DBGRANT_IDLTLY_USER,
                                       OT_DBGRANT_IDLTLN_USER,
                                       OT_DBGRANT_SESPRY_USER,
                                       OT_DBGRANT_SESPRN_USER,
                                       OT_DBGRANT_TBLSTY_USER,
                                       OT_DBGRANT_TBLSTN_USER,

                                       OT_DBGRANT_QRYCPY_USER,
                                       OT_DBGRANT_QRYCPN_USER,
                                       OT_DBGRANT_QRYPGY_USER,
                                       OT_DBGRANT_QRYPGN_USER,
                                       OT_DBGRANT_QRYCOY_USER,
                                       OT_DBGRANT_QRYCON_USER,

                                       OT_DBGRANT_SEQCRY_USER, /* these need to be the 2 last ones, since they are to*/
                                       OT_DBGRANT_SEQCRN_USER, /* appear depending on the return value ofGetOIVers() */


                                      };
#define LAYOUT_NUMBER   ( NBDBGRANTEES + 1 )

static BOOL IsGrantTypeWithValue (int granteeType)
{
  switch (granteeType) {
    case OT_DBGRANT_QRYIOY_USER:
    case OT_DBGRANT_QRYRWY_USER:
    case OT_DBGRANT_CNCTLY_USER:
    case OT_DBGRANT_IDLTLY_USER:
    case OT_DBGRANT_SESPRY_USER:
    case OT_DBGRANT_QRYCPY_USER:
    case OT_DBGRANT_QRYPGY_USER:
    case OT_DBGRANT_QRYCOY_USER:
      return TRUE;
  }
  return FALSE;
}

CuDlgDomPropDbGrantees::CuDlgDomPropDbGrantees(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropDbGrantees::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropDbGrantees)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropDbGrantees::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropDbGrantees)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropDbGrantees::AddDbGrantee (CuDbGrantee* pDbGrantee)
{
  ASSERT (pDbGrantee);
  BOOL  bSpecial = pDbGrantee->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Calculate image index according to type
  int imageIndex = 0;
  if (!bSpecial) {
    switch (pDbGrantee->GetGranteeType()) {
      case GRANTEE_TYPE_USER:
        imageIndex = 1;
        break;
      case GRANTEE_TYPE_GROUP:
        imageIndex = 2;
        break;
      case GRANTEE_TYPE_ROLE:
        imageIndex = 3;
        break;
      default:
        ASSERT (FALSE);
        imageIndex = 0;
        break;
    }
  }

  // Name
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pDbGrantee->GetStrName(), imageIndex);

  // Grantee for what ?
  for (int cpt = 0; cpt < NBDBGRANTEES; cpt++) {
	if (GetOIVers()<OIVERS_30 && cpt>=NBDBGRANTEES-2)
		break;
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, "");
    else {
      int granteeType = aGrantType[cpt];
      if (IsGrantTypeWithValue(granteeType)) {
        // display formatted text with value ONLY IF flag is set
        CString cs = _T("");
        if (pDbGrantee->GetFlag(granteeType)) {
          DWORD dwVal = pDbGrantee->GetValue(granteeType);
          cs.Format(_T("%lu"), dwVal);
        }
        m_cListCtrl.SetItemText (index, cpt+1, cs);
      }
      else {
        if (pDbGrantee->GetFlag(granteeType))
          m_cListCtrl.SetCheck (index, cpt+1, TRUE);
        else
          m_cListCtrl.SetCheck (index, cpt+1, FALSE);
        // don't disable the item,
        // since derived class CuListCtrlCheckmarks takes care of it
        //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
      }
    }
  }

  // Attach pointer on Grantee, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pDbGrantee);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropDbGrantees, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropDbGrantees)
    ON_WM_SIZE()
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDbGrantees message handlers

void CuDlgDomPropDbGrantees::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropDbGrantees::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image lists
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_DBGRANTEES); // Index 0: unknown type
  m_ImageList.AddIcon(IDI_TREE_STATIC_USER);              // Index 1: user
  m_ImageList.AddIcon(IDI_TREE_STATIC_GROUP);             // Index 2: group
  m_ImageList.AddIcon(IDI_TREE_STATIC_ROLE);              // Index 3: role
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    {   _T(""),         FALSE, 80 + 16 + 4 },  // + 16+4 since image
    {   _T("access"),    TRUE,  -1 },       //  OT_DBGRANT_ACCESY_USER,
    {   _T("no access"),    TRUE,  -1 },    //  OT_DBGRANT_ACCESN_USER,
    {   _T("crepr"),    TRUE,  -1 },        //  OT_DBGRANT_CREPRY_USER,
    {   _T("no crepr"),    TRUE,  -1 },     //  OT_DBGRANT_CREPRN_USER,
    {   _T("cretb"),    TRUE,  -1 },        //  OT_DBGRANT_CRETBY_USER,
    {   _T("no cretb"),    TRUE,  -1 },     //  OT_DBGRANT_CRETBN_USER,
    {   _T("dbadm"),    TRUE,  -1 },        //  OT_DBGRANT_DBADMY_USER,
    {   _T("no dbadm"),    TRUE,  -1 },     //  OT_DBGRANT_DBADMN_USER,
    {   _T("lkmod"),    TRUE,  -1 },        //  OT_DBGRANT_LKMODY_USER,
    {   _T("no lkmod"),    TRUE,  -1 },     //  OT_DBGRANT_LKMODN_USER,
    {   _T("qryio"),    FALSE,  -1 },        //  OT_DBGRANT_QRYIOY_USER,
    {   _T("no qryio"),    TRUE,  -1 },     //  OT_DBGRANT_QRYION_USER,
    {   _T("qryrw"),    FALSE,  -1 },        //  OT_DBGRANT_QRYRWY_USER,
    {   _T("no qryrw"),    TRUE,  -1 },     //  OT_DBGRANT_QRYRWN_USER,
    {   _T("updsc"),    TRUE,  -1 },        //  OT_DBGRANT_UPDSCY_USER,
    {   _T("no updsc"),    TRUE,  -1 },     //  OT_DBGRANT_UPDSCN_USER,
  
    {   _T("selsc"),    TRUE,  -1 },        //  OT_DBGRANT_SELSCY_USER,
    {   _T("no selsc"),    TRUE,  -1 },     //  OT_DBGRANT_SELSCN_USER,
    {   _T("cnctl"),    FALSE,  -1 },        //  OT_DBGRANT_CNCTLY_USER,
    {   _T("no cnctl"),    TRUE,  -1 },     //  OT_DBGRANT_CNCTLN_USER,
    {   _T("idltl"),    FALSE,  -1 },        //  OT_DBGRANT_IDLTLY_USER,
    {   _T("no idltl"),    TRUE,  -1 },     //  OT_DBGRANT_IDLTLN_USER,
    {   _T("sespr"),    FALSE,  -1 },        //  OT_DBGRANT_SESPRY_USER,
    {   _T("no sespr"),    TRUE,  -1 },     //  OT_DBGRANT_SESPRN_USER,
    {   _T("tblst"),    TRUE,  -1 },        //  OT_DBGRANT_TBLSTY_USER,
    {   _T("no tblst"),    TRUE,  -1 },     //  OT_DBGRANT_TBLSTN_USER,

    {   _T("qrycpu"),    FALSE,  -1 },        //  OT_DBGRANT_QRYCPY_USER,
    {   _T("no qrycpu"),    TRUE,  -1 },     //  OT_DBGRANT_QRYCPN_USER,
    {   _T("qrypage"),    FALSE,  -1 },       //  OT_DBGRANT_QRYPGY_USER,
    {   _T("no qrypage"),    TRUE,  -1 },    //  OT_DBGRANT_QRYPGN_USER,
    {   _T("qrycost"),    FALSE,  -1 },       //  OT_DBGRANT_QRYCOY_USER,
    {   _T("no qrycost"),    TRUE,  -1 },    //  OT_DBGRANT_QRYCON_USER,
    {   _T("sequence"),    TRUE,  -1 },     //  OT_DBGRANT_SEQCRY_USER,
    {   _T("no sequence"),    TRUE,  -1 }   //  OT_DBGRANT_SEQCRN_USER,

  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  InitializeColumns(m_cListCtrl, aColumns, GetOIVers()>=OIVERS_30? LAYOUT_NUMBER: LAYOUT_NUMBER-2);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropDbGrantees::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropDbGrantees::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropDbGrantees::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      case FILTER_DOM_SYSTEMOBJECTS:  // can be $ingres
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OTLL_DBGRANTEE)
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
  // Get list of Grantees
  //
  m_Data.m_uaDbGrantees.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];
  UCHAR   emptyParent[] = "";

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  switch (lpRecord->recType) {
    case OTR_DBGRANT_ACCESY_DB:
    case OTR_DBGRANT_ACCESN_DB:
    case OTR_DBGRANT_CREPRY_DB:
    case OTR_DBGRANT_CREPRN_DB:
    case OTR_DBGRANT_CRETBY_DB:
    case OTR_DBGRANT_CRETBN_DB:
    case OTR_DBGRANT_DBADMY_DB:
    case OTR_DBGRANT_DBADMN_DB:
    case OTR_DBGRANT_LKMODY_DB:
    case OTR_DBGRANT_LKMODN_DB:
    case OTR_DBGRANT_QRYIOY_DB:
    case OTR_DBGRANT_QRYION_DB:
    case OTR_DBGRANT_QRYRWY_DB:
    case OTR_DBGRANT_QRYRWN_DB:
    case OTR_DBGRANT_UPDSCY_DB:
    case OTR_DBGRANT_UPDSCN_DB :
    case OTR_DBGRANT_SELSCY_DB:
    case OTR_DBGRANT_SELSCN_DB:
    case OTR_DBGRANT_CNCTLY_DB:
    case OTR_DBGRANT_CNCTLN_DB:
    case OTR_DBGRANT_IDLTLY_DB:
    case OTR_DBGRANT_IDLTLN_DB:
    case OTR_DBGRANT_SESPRY_DB:
    case OTR_DBGRANT_SESPRN_DB:
    case OTR_DBGRANT_TBLSTY_DB:
    case OTR_DBGRANT_TBLSTN_DB:
    case OTR_DBGRANT_QRYCPY_DB:
    case OTR_DBGRANT_QRYCPN_DB:
    case OTR_DBGRANT_QRYPGY_DB:
    case OTR_DBGRANT_QRYPGN_DB:
    case OTR_DBGRANT_QRYCOY_DB:
    case OTR_DBGRANT_QRYCON_DB:
    case OTR_DBGRANT_SEQCRN_DB:
    case OTR_DBGRANT_SEQCRY_DB:
    case OTR_USERSCHEMA:
    case OT_DATABASE:
    case OTR_CDB:
      aparentsTemp[0] = lpRecord->objName;  // DBName
      break;
    case OT_STATIC_DBGRANTEES:
      aparentsTemp[0] = lpRecord->extra;    // DBName
      break;
    case OT_STATIC_INSTALL_GRANTEES:
      aparentsTemp[0] = emptyParent;        // Installation level, no Db name
      break;
    default:
      ASSERT (FALSE);
      return 0L;
  }
  aparentsTemp[1] = NULL;
  aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBDBGRANTEES; index++) {
	if (GetOIVers()<OIVERS_30 && index>=NBDBGRANTEES-2)
		break;

    iret =  DOMGetFirstObject(nNodeHandle,
                              aGrantType[index],
                              1,                            // level,
                              aparentsTemp,                 // Temp mandatory!
                              pUps->pSFilter->bWithSystem,  // bwithsystem
                              NULL,                         // lpowner
                              buf,
                              bufOwner,
                              bufComplim);
     if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
       bError = TRUE;
       continue;
    }
    else {
      while (iret == RES_SUCCESS) {
        // received data:
        // - Grantee name in buf
        // - formatted value in bufComplim if applicable
        CuDbGrantee grantee((const char *)buf,     // Grantee name (user/group/role)
                            FALSE,                 // not special item
                            aGrantType[index]     // grant type
                            );
        if (IsGrantTypeWithValue(aGrantType[index])) {
          DWORD dwValue = 0;
          sscanf((LPCTSTR)bufComplim, "%lu", &dwValue); // NOTE: value of 0 is acceptable
          grantee.SetValue(aGrantType[index], dwValue);
        }

        // Solve type on the fly and SetGranteeType
        int granteeType;
        UCHAR resBuf[MAXOBJECTNAME];
        UCHAR resBuf2[MAXOBJECTNAME];
        UCHAR resBuf3[MAXOBJECTNAME];
        int res = DOMGetObjectLimitedInfo(nNodeHandle,
                                          buf,
                                          bufOwner,
                                          OT_GRANTEE,
                                          0,     // level
                                          NULL,  // parentstrings,
                                          TRUE,
                                          &granteeType,
                                          resBuf,
                                          resBuf2,
                                          resBuf3);
        if (res != RES_SUCCESS)
          grantee.SetGranteeType(OT_ERROR);
        else
          grantee.SetGranteeType(granteeType);

        // Add, or Update existing, in the grantees array
        CuMultFlag *pRefGrantee = m_Data.m_uaDbGrantees.Find(&grantee);
        if (pRefGrantee)
          m_Data.m_uaDbGrantees.Merge(pRefGrantee, &grantee);
        else
          m_Data.m_uaDbGrantees.Add(grantee);

        iret = DOMGetNextObject(buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case 
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuDbGrantee dbGrantee1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE), TRUE);
    m_Data.m_uaDbGrantees.Add(dbGrantee1);
  }

  // Manage no grantee
  if (m_Data.m_uaDbGrantees.GetCount() == 0)
  {
	  /* "<No Grantee>" */
	  CuDbGrantee dbGrantee2(VDBA_MfcResourceString (IDS_E_NO_GRANTEE),
				 TRUE);
	  m_Data.m_uaDbGrantees.Add(dbGrantee2);
  }

  ASSERT (m_Data.m_uaDbGrantees.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDbGrantees::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataDbGrantees") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataDbGrantees* pData = (CuDomPropDataDbGrantees*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataDbGrantees)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropDbGrantees::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataDbGrantees* pData = new CuDomPropDataDbGrantees(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropDbGrantees::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaDbGrantees.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuDbGrantee* pGrantee = m_Data.m_uaDbGrantees[cnt];
    AddDbGrantee(pGrantee);
  }
  // Initial sort on names, secondary on types
  m_cListCtrl.SortItems(CuGrantee::CompareTypes, 0L);
  m_cListCtrl.SortItems(CuGrantee::CompareNames, 0L);
}

void CuDlgDomPropDbGrantees::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuGrantee::CompareNames, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 1);
      break;
  }
  *pResult = 0;
}


void CuDlgDomPropDbGrantees::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
