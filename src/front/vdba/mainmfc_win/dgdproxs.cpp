/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: dgdproxs.cpp: implementation file
**
**  Description:
**
**  History:
**  11-Apr-2003 (schph01)
**      (SIR 107523) Created
*/
#include "stdafx.h"
#include "mainmfc.h"
#include "dgdproxs.h"
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

static int aGrantedType[NBSEQGRANTED] = {
                                          OTR_GRANTEE_NEXT_SEQU,
                                        };
#define LAYOUT_NUMBER   ( NBSEQGRANTED + 3 )

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRoleXGrantedSeq dialog

CuDlgDomPropRoleXGrantedSeq::CuDlgDomPropRoleXGrantedSeq(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropRoleXGrantedSeq::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropRoleXGrantedSeq)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDomPropRoleXGrantedSeq::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropRoleXGrantedSeq)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropRoleXGrantedSeq, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropRoleXGrantedSeq)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRoleXGrantedSeq message handlers
void CuDlgDomPropRoleXGrantedSeq::AddRoleXGrantedSeq (CuGrantedSeq* pGrantedSeq)
{
	ASSERT (pGrantedSeq);
	BOOL  bSpecial = pGrantedSeq->IsSpecialItem();

	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();

	// Name
	index  = m_cListCtrl.InsertItem (nCount, pGrantedSeq->GetStrName(), 0);

	// Owner - parent Seq
	m_cListCtrl.SetItemText (index, 1, (LPCSTR)pGrantedSeq->GetOwnerName());
	m_cListCtrl.SetItemText (index, 2, (LPCSTR)pGrantedSeq->GetParentName());

	for (int cpt = 0; cpt < NBSEQGRANTED; cpt++) {
	if (bSpecial)
		m_cListCtrl.SetItemText (index, cpt+3, _T(""));
	else {
		if (pGrantedSeq->GetFlag(aGrantedType[cpt]))
			m_cListCtrl.SetCheck (index, cpt+3, TRUE);
		else
			m_cListCtrl.SetCheck (index, cpt+3, FALSE);
		// don't disable the item,
		// since derived class CuListCtrlCheckmarks takes care of it
		//m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
	}
	}

	// Attach pointer on granted object, for future sort purposes
	m_cListCtrl.SetItemData(index, (DWORD)pGrantedSeq);
}

void CuDlgDomPropRoleXGrantedSeq::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}

LONG CuDlgDomPropRoleXGrantedSeq::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropRoleXGrantedSeq::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OT_SEQUENCE)
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
  m_Data.m_uaRoleXGrantedSeq.RemoveAll();

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
  for (int index = 0; index < NBSEQGRANTED; index++) {
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
        CuGrantedSeq grantedSeq((const char *)buf,                // obj name
                                (const char *)bufOwner,           // obj owner
                                (const char *)aparentsResult[0],  // obj parent
                                aGrantedType[index]               // grant type
                               );
        CuMultFlag *pRefGrantedSeq = m_Data.m_uaRoleXGrantedSeq.Find(&grantedSeq);
        if (pRefGrantedSeq)
          m_Data.m_uaRoleXGrantedSeq.Merge(pRefGrantedSeq, &grantedSeq);
        else
          m_Data.m_uaRoleXGrantedSeq.Add(grantedSeq);

        iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuGrantedSeq grantedSeq1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaRoleXGrantedSeq.Add(grantedSeq1);
  }

  // Manage no granted object
  if (m_Data.m_uaRoleXGrantedSeq.GetCount() == 0)
  {
    /* "<No GrantedSeq>" */
    CuGrantedSeq grantedSeq2(VDBA_MfcResourceString (IDS_E_NO_GRANTED_SEQ));
    m_Data.m_uaRoleXGrantedSeq.Add(grantedSeq2);
  }

  ASSERT (m_Data.m_uaRoleXGrantedSeq.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRoleXGrantedSeq::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataRoleXGrantedSeq") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataRoleXGrantedSeq* pData = (CuDomPropDataRoleXGrantedSeq*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataRoleXGrantedSeq)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropRoleXGrantedSeq::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataRoleXGrantedSeq* pData = new CuDomPropDataRoleXGrantedSeq(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropRoleXGrantedSeq::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CuDlgDomPropRoleXGrantedSeq::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropRoleXGrantedSeq::OnInitDialog() 
{
  CDialog::OnInitDialog();
  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_SEQUENCE);
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T(""), FALSE, 80 + 16+4 }, // 16+4 for image width
    { _T(""), FALSE, 80 },
    { _T(""), FALSE, 80 },
    { _T(""), TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_OWNER));//_T("Owner")
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_PARENT_DB));
  lstrcpy(aColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_NEXT_ON_SEQ));//_T("Next on Sequence")

  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);
  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropRoleXGrantedSeq::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaRoleXGrantedSeq.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuGrantedSeq* pGrantedSeq = m_Data.m_uaRoleXGrantedSeq[cnt];
    AddRoleXGrantedSeq(pGrantedSeq);
  }
  // No Initial sort
}

void CuDlgDomPropRoleXGrantedSeq::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
