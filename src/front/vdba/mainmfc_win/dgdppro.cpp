// DgDPPro.cpp : implementation file
//
/*
** History >= 10-Sep-2004
**
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdppro.h"

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
static const int LAYOUT_NUMBER = 3;

IMPLEMENT_DYNCREATE(CuDlgDomPropProfile, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProfile dialog

//
// NOTE: m_profile (0, TRUE, NULL) used here is to expose the list of preveleges only
CuDlgDomPropProfile::CuDlgDomPropProfile()
  : CFormView(CuDlgDomPropProfile::IDD), m_profile (0, TRUE, NULL)
{
  //{{AFX_DATA_INIT(CuDlgDomPropProfile)
  //}}AFX_DATA_INIT
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_PROFILE);
	m_bNeedSubclass = TRUE;
}

CuDlgDomPropProfile::~CuDlgDomPropProfile()
{
}

void CuDlgDomPropProfile::DoDataExchange(CDataExchange* pDX)
{
  CFormView::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropProfile)
  DDX_Control(pDX, IDC_EDIT_LIMITSECLABEL, m_edLimSecureLabel);
  DDX_Control(pDX, IDC_EDIT_EXPIREDATE, m_edExpireDate);
  DDX_Control(pDX, IDC_EDIT_DEFGROUP, m_edDefGroup);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropProfile, CFormView)
  //{{AFX_MSG_MAP(CuDlgDomPropProfile)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProfile diagnostics

#ifdef _DEBUG
void CuDlgDomPropProfile::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropProfile::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropProfile message handlers

void CuDlgDomPropProfile::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropProfile::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropProfile::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
  PROFILEPARAMS profileParams;
  memset (&profileParams, 0, sizeof (profileParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  x_strcpy ((char *)profileParams.ObjectName, (const char *)lpRecord->objName);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_PROFILE,
                               &profileParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataProfile tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  if (profileParams.DefaultGroup[0])
    m_Data.m_csDefGroup = profileParams.DefaultGroup;
  else
    m_Data.m_csDefGroup.LoadString(IDS_DEFAULT_GROUP);// = "< No Default Group >";
  m_Data.m_csExpireDate = profileParams.ExpireDate;
  if (profileParams.lpLimitSecLabels)
    m_Data.m_csLimSecureLabel = profileParams.lpLimitSecLabels;
  else
    m_Data.m_csLimSecureLabel = "";

  ASSERT (sizeof(m_Data.m_aPrivilege) == sizeof (profileParams.Privilege) );
  memcpy(m_Data.m_aPrivilege, profileParams.Privilege, sizeof (profileParams.Privilege));
  ASSERT (sizeof(m_Data.m_aDefaultPrivilege) == sizeof (profileParams.bDefaultPrivilege) );
  memcpy(m_Data.m_aDefaultPrivilege, profileParams.bDefaultPrivilege, sizeof (profileParams.bDefaultPrivilege));
  ASSERT (sizeof(m_Data.m_aSecAudit) == sizeof (profileParams.bSecAudit) );
  memcpy(m_Data.m_aSecAudit, profileParams.bSecAudit, sizeof (profileParams.bSecAudit));

  // liberate detail structure
  FreeAttachedPointers (&profileParams, OT_PROFILE);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropProfile::OnLoad (WPARAM wParam, LPARAM lParam)
{
  SubclassControls();
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataProfile") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataProfile* pData = (CuDomPropDataProfile*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataProfile)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropProfile::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataProfile* pData = new CuDomPropDataProfile(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropProfile::RefreshDisplay()
{
	SubclassControls();
	UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

	//
	// m_cListPrivilege must have been subclassed !!!
	ASSERT (m_bNeedSubclass == FALSE);
	if (!m_bNeedSubclass)
	{
		int i=0;
		CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = m_profile.m_listPrivileges;
		POSITION pos = lp.GetHeadPosition();
		while (pos != NULL && i<USERPRIVILEGES)
		{
			CaURPPrivilegesItemData* pItem = lp.GetNext (pos);
			pItem->m_bUser = m_Data.m_aPrivilege[i];
			pItem->m_bDefault = m_Data.m_aDefaultPrivilege[i];
			i++;
		}
		m_cListPrivilege.DisplayPrivileges();
	}

  m_edDefGroup.SetWindowText(m_Data.m_csDefGroup);
  m_edExpireDate.SetWindowText(m_Data.m_csExpireDate);
  m_edLimSecureLabel.SetWindowText(m_Data.m_csLimSecureLabel);
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(m_Data.m_aSecAudit[USERALLEVENT]? 1: 0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(m_Data.m_aSecAudit[USERQUERYTEXTEVENT]? 1: 0);

}

void CuDlgDomPropProfile::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edDefGroup.SetWindowText(_T(""));
  m_edExpireDate.SetWindowText(_T(""));
  m_edLimSecureLabel.SetWindowText(_T(""));
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(0);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

void CuDlgDomPropProfile::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	SubclassControls();
	if (GetOIVers() >= OIVERS_30)
	{
		m_edLimSecureLabel.ShowWindow(SW_HIDE);
		CWnd* pWnd = GetDlgItem(IDC_MFC_STATIC1);
		if (pWnd && pWnd->m_hWnd)
			pWnd->ShowWindow(SW_HIDE);
	}
}

void CuDlgDomPropProfile::SubclassControls()
{
	// Subclass HERE!
	if (m_bNeedSubclass) 
	{
		m_cListPrivilege.m_bReadOnly = TRUE;
		VERIFY (m_cListPrivilege.SubclassDlgItem (IDC_MFC_LIST1, this));
		m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
		m_cListPrivilege.SetCheckImageList(&m_ImageCheck);
		m_bNeedSubclass = FALSE;

		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		//
		LV_COLUMN lvcolumn;
		LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
		{
			{IDS_URP_PRIVILEGE,   160,  LVCFMT_LEFT,   FALSE},
			{IDS_URP_USER,         50,  LVCFMT_CENTER, TRUE },
			{IDS_URP_DEFAULT,      60,  LVCFMT_CENTER, TRUE },
		};

		CString strHeader;
		memset (&lvcolumn, 0, sizeof (LV_COLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (int i=0; i<LAYOUT_NUMBER; i++)
		{
			strHeader.LoadString (lsp[i].m_nIDS);
			lvcolumn.fmt      = lsp[i].m_fmt;
			lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx       = lsp[i].m_cxWidth;
			m_cListPrivilege.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
		}

		FillPrivileges();
	}
}

void CuDlgDomPropProfile::FillPrivileges()
{
	TCHAR tchszText[] =_T("");
	int nCount = 0;
	LV_ITEM lvitem;
	CaURPPrivilegesItemData* pItem = NULL;
	CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = m_profile.m_listPrivileges;
	memset (&lvitem, 0, sizeof(lvitem));
	m_cListPrivilege.DeleteAllItems();
	POSITION pos = lp.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = lp.GetNext (pos);

		lvitem.mask   = LVIF_PARAM|LVIF_TEXT;
		lvitem.pszText= tchszText;
		lvitem.iItem  = nCount;
		lvitem.lParam = (LPARAM)pItem;

		m_cListPrivilege.InsertItem (&lvitem);
		nCount++;
	}
	m_cListPrivilege.DisplayPrivileges();
}
