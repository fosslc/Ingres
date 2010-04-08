/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpEvGr.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_GRANTEE into two statements to avoid compiler error.
**  12-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpevgr.h"
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

static int aGrantType[NBDBEVGRANTEES] = { OT_DBEGRANT_RAISE_USER,
                                          OT_DBEGRANT_REGTR_USER, };
#define LAYOUT_NUMBER   ( NBDBEVGRANTEES + 1 )

CuDlgDomPropDbEventGrantees::CuDlgDomPropDbEventGrantees(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropDbEventGrantees::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropDbEventGrantees)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropDbEventGrantees::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropDbEventGrantees)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropDbEventGrantees::AddDbEventGrantee (CuDbEventGrantee* pDbEventGrantee)
{
  ASSERT (pDbEventGrantee);
  BOOL  bSpecial = pDbEventGrantee->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Calculate image index according to type
  int imageIndex = 0;
  if (!bSpecial) {
    switch (pDbEventGrantee->GetGranteeType()) {
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
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pDbEventGrantee->GetStrName(), imageIndex);

  // Grantee for what ?
  for (int cpt = 0; cpt < NBDBEVGRANTEES; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, "");
    else {
      if (pDbEventGrantee->GetFlag(aGrantType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+1, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+1, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on Grantee, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pDbEventGrantee);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropDbEventGrantees, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropDbEventGrantees)
    ON_WM_SIZE()
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDbEventGrantees message handlers

void CuDlgDomPropDbEventGrantees::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropDbEventGrantees::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
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
    {  _T(""),      FALSE, 100 + 16 + 4 },  // + 16+4 since image
    {  _T("Raise"),     TRUE,  -1 },
    {  _T("Register"),  TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropDbEventGrantees::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropDbEventGrantees::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropDbEventGrantees::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OTLL_GRANTEE)
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
  m_Data.m_uaDbEventGrantees.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->extra;  // DBName

  UCHAR bufParent1[MAXOBJECTNAME];
  aparentsTemp[1] = bufParent1;
  x_strcpy((char *)buf, (const char *)lpRecord->objName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[1]); // schema.name

  aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBDBEVGRANTEES; index++) {
    iret =  DOMGetFirstObject(nNodeHandle,
                              aGrantType[index],
                              2,                            // level,
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
        CuDbEventGrantee grantee((const char *)buf,     // Grantee name (user/group/role)
                               FALSE,                 // not special item
                               aGrantType[index]    // grant type
                               );

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

        CuMultFlag *pRefGrantee = m_Data.m_uaDbEventGrantees.Find(&grantee);
        if (pRefGrantee)
          m_Data.m_uaDbEventGrantees.Merge(pRefGrantee, &grantee);
        else
          m_Data.m_uaDbEventGrantees.Add(grantee);

        iret = DOMGetNextObject(buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case 
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuDbEventGrantee grantee1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE),
			      TRUE);
    m_Data.m_uaDbEventGrantees.Add(grantee1);
  }

  // Manage no grantee
  if (m_Data.m_uaDbEventGrantees.GetCount() == 0)
  {
    /* "<No Grantee>" */
    CuDbEventGrantee grantee2(VDBA_MfcResourceString (IDS_E_NO_GRANTEE), TRUE);
    m_Data.m_uaDbEventGrantees.Add(grantee2);
  }

  ASSERT (m_Data.m_uaDbEventGrantees.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDbEventGrantees::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataDbEventGrantees") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataDbEventGrantees* pData = (CuDomPropDataDbEventGrantees*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataDbEventGrantees)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropDbEventGrantees::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataDbEventGrantees* pData = new CuDomPropDataDbEventGrantees(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropDbEventGrantees::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaDbEventGrantees.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuDbEventGrantee* pGrantee = m_Data.m_uaDbEventGrantees[cnt];
    AddDbEventGrantee(pGrantee);
  }
  // Initial sort on names, secondary on types
  m_cListCtrl.SortItems(CuGrantee::CompareTypes, 0L);
  m_cListCtrl.SortItems(CuGrantee::CompareNames, 0L);
}

void CuDlgDomPropDbEventGrantees::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgDomPropDbEventGrantees::ResetDisplay()
{
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
