// DgDPRol.cpp : implementation file
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

#include "dgdprol.h"

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
static const int LAYOUT_NUMBER = 2;


IMPLEMENT_DYNCREATE(CuDlgDomPropRole, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRole dialog

//
// NOTE: m_profile (0, TRUE, NULL) used here is to expose the list of preveleges only
CuDlgDomPropRole::CuDlgDomPropRole()
	: CFormView(CuDlgDomPropRole::IDD), m_profile (0, TRUE, NULL)
{
	//{{AFX_DATA_INIT(CuDlgDomPropRole)
	//}}AFX_DATA_INIT
	m_Data.m_refreshParams.InitializeRefreshParamsType(OT_ROLE);
	m_bNeedSubclass = TRUE;
}

CuDlgDomPropRole::~CuDlgDomPropRole()
{
}

void CuDlgDomPropRole::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropRole)
	DDX_Control(pDX, IDC_LIST_GRANTEES, m_lbGranteesOnRole);
	DDX_Control(pDX, IDC_EDIT_LIMITSECLABEL, m_edLimSecureLabel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropRole, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropRole)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRole diagnostics

#ifdef _DEBUG
void CuDlgDomPropRole::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropRole::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRole message handlers

void CuDlgDomPropRole::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropRole::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropRole::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_SYSTEMOBJECTS:    // because list of grantees can contain "$ingres"
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
  ROLEPARAMS roleParams;
  memset (&roleParams, 0, sizeof (roleParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Manage "Public" special case
  //
  CString csPublic(lppublicdispstring());
  if (csPublic.CompareNoCase ((const char *)lpRecord->objName) == 0) {
    // Unacceptable for this property page since managed differently in AllocateUserPageInfo() (mainvi2.cpp)
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataRole tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // starting from here, "Normal" case

  //
  // Get Detail Info
  //
  x_strcpy ((char *)roleParams.ObjectName, (const char *)lpRecord->objName);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_ROLE,
                               &roleParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataRole tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  if (roleParams.lpLimitSecLabels)
    m_Data.m_csLimSecureLabel = roleParams.lpLimitSecLabels;
  else
    m_Data.m_csLimSecureLabel = "";

  ASSERT (sizeof(m_Data.m_aPrivilege) == sizeof (roleParams.Privilege) );
  memcpy(m_Data.m_aPrivilege, roleParams.Privilege, sizeof (roleParams.Privilege));
  ASSERT (sizeof(m_Data.m_aSecAudit) == sizeof (roleParams.bSecAudit) );
  memcpy(m_Data.m_aSecAudit, roleParams.bSecAudit, sizeof (roleParams.bSecAudit));

  // liberate detail structure
  FreeAttachedPointers (&roleParams, OT_ROLE);

  //
  // Get list of grantees on role
  //
  m_Data.m_acsGranteesOnRole.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_ROLEGRANT_USER,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CString csUnavail = VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE);//"<Data Unavailable>"
    m_Data.m_acsGranteesOnRole.Add(csUnavail);
  }
  else {
    while (iret == RES_SUCCESS) {
      m_Data.m_acsGranteesOnRole.Add((LPCTSTR)buf);
      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_acsGranteesOnRole.GetUpperBound() == -1) {
    CString csNoGrantee = VDBA_MfcResourceString (IDS_E_NO_GRANTEE);//"<No Grantee>";
    m_Data.m_acsGranteesOnRole.Add(csNoGrantee);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRole::OnLoad (WPARAM wParam, LPARAM lParam)
{
  SubclassControls();
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataRole") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataRole* pData = (CuDomPropDataRole*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataRole)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRole::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataRole* pData = new CuDomPropDataRole(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropRole::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

	SubclassControls();
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
			i++;
		}
		m_cListPrivilege.DisplayPrivileges();
	}

  m_edLimSecureLabel.SetWindowText(m_Data.m_csLimSecureLabel);
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(m_Data.m_aSecAudit[USERALLEVENT]? 1: 0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(m_Data.m_aSecAudit[USERQUERYTEXTEVENT]? 1: 0);

  int cnt;
  int size;

  m_lbGranteesOnRole.ResetContent();
  size = m_Data.m_acsGranteesOnRole.GetSize();
  for (cnt = 0; cnt < size; cnt++)
    m_lbGranteesOnRole.AddString(m_Data.m_acsGranteesOnRole[cnt]);

}

void CuDlgDomPropRole::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edLimSecureLabel.SetWindowText(_T(""));
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(0);

  m_lbGranteesOnRole.ResetContent();
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

void CuDlgDomPropRole::OnInitialUpdate() 
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


void CuDlgDomPropRole::SubclassControls()
{
	// Subclass HERE!
	if (m_bNeedSubclass) 
	{
		m_cListPrivilege.m_bReadOnly = TRUE;
//		m_cListPrivilege.SetLineSeperator(FALSE);
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
			{IDS_URP_PRIVILEGE,   156,  LVCFMT_LEFT,   FALSE},
			{IDS_URP_ROLE,         60,  LVCFMT_CENTER, TRUE },
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

void CuDlgDomPropRole::FillPrivileges()
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
