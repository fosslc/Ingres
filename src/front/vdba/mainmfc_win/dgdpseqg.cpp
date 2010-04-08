/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  dgdpseqg.cpp : implementation file
**
** 02-Apr-2003 (schph01)
**    SIR #107523, Display the list of grantees for the sequence in property pane.
**
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpseqg.h"
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

static int aGrantType[NBPROCGRANTEES] = { OT_SEQUGRANT_NEXT_USER, };
#define LAYOUT_NUMBER   ( NBPROCGRANTEES + 1 )

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeqGrantees dialog

CuDlgDomPropSeqGrantees::CuDlgDomPropSeqGrantees(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropSeqGrantees::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropSeqGrantees)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDomPropSeqGrantees::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropSeqGrantees)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgDomPropSeqGrantees::AddSeqGrantee (CuSeqGrantee* pSeqGrantee)
{
  ASSERT (pSeqGrantee);
  BOOL  bSpecial = pSeqGrantee->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Calculate image index according to type
  int imageIndex = 0;
  if (!bSpecial) {
    switch (pSeqGrantee->GetGranteeType()) {
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
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pSeqGrantee->GetStrName(), imageIndex);

  // Grantee for what ?
  for (int cpt = 0; cpt < NBPROCGRANTEES; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, "");
    else {
      if (pSeqGrantee->GetFlag(aGrantType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+1, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+1, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on Grantee, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pSeqGrantee);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropSeqGrantees, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropSeqGrantees)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeqGrantees message handlers
void CuDlgDomPropSeqGrantees::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

BOOL CuDlgDomPropSeqGrantees::OnInitDialog() 
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
	{  _T(""),        FALSE, 100 + 16 + 4 },  // + 16+4 since image
	{  _T(""),        TRUE,  -1 },
	};
	lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));   // _T("Name")
	lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_NEXT));   // _T("Next")
	InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropSeqGrantees::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
	
}

LONG CuDlgDomPropSeqGrantees::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_LIST;
}


LONG CuDlgDomPropSeqGrantees::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get list of Grantees
  //
  m_Data.m_uaSeqGrantees.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->extra;  // parent DB Name

  UCHAR bufParent1[MAXOBJECTNAME];
  aparentsTemp[1] = bufParent1;
  switch (lpRecord->recType) {
    case OT_SEQUENCE:
    case OTR_GRANTEE_NEXT_SEQU:
      x_strcpy((char *)buf, (const char *)lpRecord->objName);
      break;
    case OT_SEQUGRANT_NEXT_USER:
    case OT_STATIC_SEQGRANT_NEXT_USER:
      x_strcpy((char *)buf, (const char *)lpRecord->extra2);
      break;
    default:
      ASSERT (FALSE);
      buf[0] = '\0';
  }
  ASSERT (lpRecord->ownerName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[1]); // schema.name

  aparentsTemp[2] = NULL;

  // loop on grantees
  BOOL bError = FALSE;
  for (int index = 0; index < NBPROCGRANTEES; index++) {
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
        CuSeqGrantee grantee  ((const char *)buf,     // Grantee name (user/group/role)
                               FALSE,                 // not special item
                               aGrantType[index]      // grant type
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

        CuMultFlag *pRefGrantee = m_Data.m_uaSeqGrantees.Find(&grantee);
        if (pRefGrantee)
          m_Data.m_uaSeqGrantees.Merge(pRefGrantee, &grantee);
        else
          m_Data.m_uaSeqGrantees.Add(grantee);

        iret = DOMGetNextObject(buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case 
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuSeqGrantee grantee1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE), TRUE);
    m_Data.m_uaSeqGrantees.Add(grantee1);
  }

  // Manage no grantee
  if (m_Data.m_uaSeqGrantees.GetCount() == 0)
  {
    /* "<No Grantee>" */
    CuSeqGrantee grantee2(VDBA_MfcResourceString (IDS_E_NO_GRANTEE), TRUE);
    m_Data.m_uaSeqGrantees.Add(grantee2);
  }

  ASSERT (m_Data.m_uaSeqGrantees.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropSeqGrantees::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataSeqGrantees") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataSeqGrantees* pData = (CuDomPropDataSeqGrantees*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataSeqGrantees)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropSeqGrantees::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataSeqGrantees* pData = new CuDomPropDataSeqGrantees(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropSeqGrantees::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgDomPropSeqGrantees::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaSeqGrantees.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuSeqGrantee* pGrantee = m_Data.m_uaSeqGrantees[cnt];
    AddSeqGrantee(pGrantee);
  }
  // Initial sort on names, secondary on types
  m_cListCtrl.SortItems(CuGrantee::CompareTypes, 0L);
  m_cListCtrl.SortItems(CuGrantee::CompareNames, 0L);
}

void CuDlgDomPropSeqGrantees::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

