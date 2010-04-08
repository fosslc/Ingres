/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rvstartu.cpp, Implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Startup Setting)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 20-May-1999 (schph01)
**    Change the column width in the CListCtrl (CuListCtrl).
** 24-Jun-2002 (hanje04)
**    Cast all CStrings being passed to other functions or methods using %s
**    with (LPCTSTR) and made calls more readable on UNIX.
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "rvstartu.h"
#include "ipmprdta.h"
#include "monrepli.h"
#include "ipmframe.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationServerPageStartupSetting::CuDlgReplicationServerPageStartupSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationServerPageStartupSetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationServerPageStartupSetting)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationServerPageStartupSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationServerPageStartupSetting)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgReplicationServerPageStartupSetting::AddItem(CaReplicationItem* pItem)
{
	int nCount = m_cListCtrl.GetItemCount();
	int nIndex = m_cListCtrl.InsertItem (nCount, pItem->GetDescription());
	if (nIndex != -1)
	{
		BOOL bDefault;
		if (pItem->GetType() == CaReplicationItem::REP_BOOLEAN)
		{
			CString strValue = pItem->GetFlagContent(bDefault);
			if (strValue.CompareNoCase (_T("TRUE")) == 0)
				m_cListCtrl.SetCheck (nIndex, 1, TRUE);
			else
				m_cListCtrl.SetCheck (nIndex, 1, FALSE);
		}
		else
			m_cListCtrl.SetItemText (nIndex, 1, pItem->GetFlagContent(bDefault));
		m_cListCtrl.SetItemText (nIndex, 2, pItem->GetUnit());
		m_cListCtrl.SetItemData (nIndex, (DWORD)pItem);
	}
}


BEGIN_MESSAGE_MAP(CuDlgReplicationServerPageStartupSetting, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationServerPageStartupSetting)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSave)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonEditValue)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonRestore)
	ON_BN_CLICKED(IDC_BUTTON4, OnButtonRestoreAll)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_WM_DESTROY()
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
	ON_MESSAGE (WM_LAYOUTEDIT_HIDE_PROPERTY, OnHideProperty)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnLeavingPage)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,         OnDataChanged)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,     OnPropertiesChange)
END_MESSAGE_MAP()

void CuDlgReplicationServerPageStartupSetting::Cleanup()
{
	int i, nCount = m_cListCtrl.GetItemCount();
	CaReplicationItem* pItem = NULL;

	for (i=0; i<nCount; i++)
	{
		pItem = (CaReplicationItem*)m_cListCtrl.GetItemData (i);
		if (pItem)
			delete pItem;
	}
}

void CuDlgReplicationServerPageStartupSetting::EnableButtons()
{
	BOOL bEnableSaveButton      = FALSE;
	BOOL bEnableEditButton      = FALSE;
	BOOL bEnableRestorButton    = FALSE;
	BOOL bEnableRestoreAllButton= FALSE;

	if (m_cListCtrl.GetItemCount() > 0)
	{
		bEnableRestoreAllButton = TRUE;
		int nCount = m_cListCtrl.GetItemCount();
		for (int i=0; i<nCount; i++)
		{
			CaReplicationItem* pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(i);
			if (pItem->IsValueModifyByUser())
			{
				bEnableSaveButton = TRUE;
				break;
			}
		}
		int nSelected = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
		if (nSelected != -1)
		{
			CaReplicationItem* pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(nSelected);
			if (!pItem->IsReadOnlyFlag())
			{
				bEnableRestorButton = TRUE;
				bEnableEditButton   = TRUE;
			}
		}
	}
	GetDlgItem(IDC_BUTTON1)->EnableWindow (bEnableSaveButton);
	GetDlgItem(IDC_BUTTON2)->EnableWindow (bEnableEditButton);
	GetDlgItem(IDC_BUTTON3)->EnableWindow (bEnableRestorButton);
	GetDlgItem(IDC_BUTTON4)->EnableWindow (bEnableRestoreAllButton);
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationServerPageStartupSetting message handlers

void CuDlgReplicationServerPageStartupSetting::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	EnableButtons();
	*pResult = 0;
}

void CuDlgReplicationServerPageStartupSetting::OnDestroy() 
{
	Cleanup();
	CDialog::OnDestroy();
}

LONG CuDlgReplicationServerPageStartupSetting::OnHideProperty (WPARAM wParam, LPARAM lParam)
{
	m_cListCtrl.HideProperty();
	return 0L;
}

void CuDlgReplicationServerPageStartupSetting::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationServerPageStartupSetting::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create(1, 18, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	// If modify this constant and the column width, make sure do not forget
	// to port to the OnLoad() and OnGetData() members.
	const int LAYOUT_NUMBER = 3;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_PARAM, 200,  LVCFMT_LEFT,FALSE},
		{IDS_TC_VALUE, 100,  LVCFMT_LEFT,TRUE},
		{IDS_TC_UNIT,   80,  LVCFMT_LEFT,FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationServerPageStartupSetting::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cListCtrl.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right  - r.left;
	r.bottom = rDlg.bottom - r.left;
	m_cListCtrl.MoveWindow (r);
}

void CuDlgReplicationServerPageStartupSetting::OnButtonSave() 
{
	int iret;
	CWaitCursor hourglass;

	CdIpmDoc* pIpmDoc = NULL;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
	ASSERT(pIpmFrame);
	if (pIpmFrame)
	{
		pIpmDoc = pIpmFrame->GetIpmDoc();
		ASSERT (pIpmDoc);
	}
	if (!pIpmDoc)
		return;

	ASSERT (m_FlagsList.IsEmpty());
	while (!m_FlagsList.IsEmpty())
		delete m_FlagsList.RemoveHead();
	int i, nCount = m_cListCtrl.GetItemCount();
	CaReplicationItem* pItem = NULL;
	for (i=0; i<nCount; i++)    {
		pItem = (CaReplicationItem*)m_cListCtrl.GetItemData (i);
		if (pItem)
			m_FlagsList.AddTail (pItem);
	}

	CString csVnodeAndUsers;
	int nNodeHdl = -1;
	if (lstrcmp((char *)m_pSvrDta->LocalDBNode,(char *)m_pSvrDta->RunNode) == 0)
		csVnodeAndUsers.Format(_T("%s%s%s%s"), m_pSvrDta->LocalDBNode,
		                                   LPUSERPREFIXINNODENAME,
		                                   (LPCTSTR)m_csDBAOwner,
										   LPUSERSUFFIXINNODENAME);
	else
		csVnodeAndUsers = m_pSvrDta->RunNode;

	nNodeHdl  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csVnodeAndUsers);
	if (nNodeHdl == -1) {
		CString strMsg,strMsg1;
		strMsg.LoadString(IDS_MAX_NB_CONNECT);   // _T("Maximum number of connections has been reached"
		strMsg1.LoadString(IDS_E_SAVE_FILE);     //    " - Cannot save file."
		strMsg += _T("\n")+strMsg1;
		AfxMessageBox(strMsg, NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return;
	}
	iret = SetReplicServerParams ( pIpmDoc, m_pSvrDta, &m_FlagsList );
	LIBMON_CloseNodeStruct (nNodeHdl);

	if ( iret == RES_SUCCESS )
		m_FlagsList.SetModifyEdit(); // Not modify.
	if (!m_FlagsList.IsEmpty())
		m_FlagsList.RemoveAll();

	EnableButtons();
}

void CuDlgReplicationServerPageStartupSetting::OnButtonEditValue()
{
	CRect r, rCell;
	int iNameCol = 1;
	int iIndex = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);

	if (iIndex == -1)
		return;
	CaReplicationItem* pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(iIndex);
	if (!pItem)
		return;
	if (pItem->GetType() == CaReplicationItem::REP_BOOLEAN)
		return;
	m_cListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_cListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_cListCtrl.EditValue (iIndex, iNameCol, rCell);
	}
}


void CuDlgReplicationServerPageStartupSetting::OnButtonRestore()
{
	UINT nState = 0;
	CString strMessage;
	int answ, nCount = m_cListCtrl.GetItemCount(),iIndex = -1;

	m_cListCtrl.HideProperty(TRUE);
	iIndex = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);

	if (iIndex == -1)
		return;
	CaReplicationItem* pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(iIndex);
	if (!pItem)
		return;
	//"Do you want to restore '%s' "
	//"to its default setting ?"
	strMessage.Format(IDS_F_RESTORE_DEFAULT,(LPCTSTR)pItem->GetDescription());
	answ = AfxMessageBox(strMessage , MB_ICONEXCLAMATION | MB_YESNO);
	if ( answ == IDYES ){
		BOOL bNotUsed;
		pItem->SetToDefault();
		pItem->SetValueModifyByUser(TRUE);
		if (pItem->GetType() == CaReplicationItem::REP_BOOLEAN)
		{
			CString strValue = pItem->GetFlagContent(bNotUsed);
			if (strValue.CompareNoCase (_T("TRUE")) == 0)
				m_cListCtrl.SetCheck (iIndex, 1, TRUE);
			else
				m_cListCtrl.SetCheck (iIndex, 1, FALSE);
		}
		else
			m_cListCtrl.SetItemText (iIndex, 1, pItem->GetFlagContent(bNotUsed));
	}
	EnableButtons();
}

void CuDlgReplicationServerPageStartupSetting::OnButtonRestoreAll()
{
	CString strMessage;
	int i,answ, nCount = m_cListCtrl.GetItemCount();
	if (nCount == 0)
		return;
	m_cListCtrl.HideProperty(TRUE);
	answ = AfxMessageBox(IDS_A_RESTORE_ALL , MB_ICONEXCLAMATION | MB_YESNO );
	if ( answ == IDYES ) {
		for (i=0; i<nCount; i++)
		{
			CaReplicationItem* pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(i);
			if ( pItem != NULL)
			{
				BOOL bNotUsed;
				pItem->SetToDefault();
				pItem->SetValueModifyByUser(TRUE);
				if (pItem->GetType() == CaReplicationItem::REP_BOOLEAN)
				{
					CString strValue = pItem->GetFlagContent(bNotUsed);
					if (strValue.CompareNoCase (_T("TRUE")) == 0)
						m_cListCtrl.SetCheck (i, 1, TRUE);
					else
						m_cListCtrl.SetCheck (i, 1, FALSE);
				}
				else
					m_cListCtrl.SetItemText (i, 1, pItem->GetFlagContent(bNotUsed));
			}
		}
	}
	EnableButtons();
}



LONG CuDlgReplicationServerPageStartupSetting::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int iret,ires,irestype, nNodeHdl = -1;
	UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
	UCHAR buf[EXTRADATASIZE];
	UCHAR extradata[EXTRADATASIZE];
	LPUCHAR parentstrings [MAXPLEVEL];
	CString cDefNumSvr,cDefDbName,cLocal;
	CString strMsg,strMsg1;

	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	m_pSvrDta = (LPREPLICSERVERDATAMIN)pUps->pStruct;
	EnableButtons();
	//
	// Specialize the OnUpdateData:
	if (!m_pSvrDta->m_bRefresh)
		return 0L;
	m_pSvrDta->m_bRefresh = FALSE;

	CdIpmDoc* pIpmDoc = NULL;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
	ASSERT(pIpmFrame);
	if (pIpmFrame)
	{
		pIpmDoc = pIpmFrame->GetIpmDoc();
		ASSERT (pIpmDoc);
	}
	if (!pIpmDoc)
		return 0L;

	nNodeHdl  = LIBMON_OpenNodeStruct (m_pSvrDta->LocalDBNode);
	if (nNodeHdl == -1) {
		strMsg.LoadString(IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
		strMsg1.LoadString (IDS_E_READ_FILE);   //    " - Cannot read file."
		strMsg += _T("\n") + strMsg1;
		MessageBox(strMsg ,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return 0L;
	}

	// Temporary for activate a session
	ires = DOMGetFirstObject (nNodeHdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	cDefDbName.Format(_T("%s::%s"), m_pSvrDta->LocalDBNode,m_pSvrDta->LocalDBName);
	//
	//Get DBA user name for this database
	parentstrings[0]=m_pSvrDta->LocalDBName;
	parentstrings[1]=NULL;
	memset (DBAUsernameOntarget,'\0',sizeof(DBAUsernameOntarget));
	iret = DOMGetObjectLimitedInfo( nNodeHdl,
	                                parentstrings [0],
	                                (UCHAR *)"",
	                                OT_DATABASE,
	                                0,
	                                parentstrings,
	                                TRUE,
	                                &irestype,
	                                buf,
	                                DBAUsernameOntarget,
	                                extradata
	                              );
	if (iret != RES_SUCCESS) {
		LIBMON_CloseNodeStruct(nNodeHdl);
		//wsprintf((char *)buf,"DBA owner on database : %s not found. Read file aborted.",parentstrings[0]);
		strMsg.Format(IDS_F_DB_OWNER,parentstrings[0]);
		MessageBox(strMsg, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return 0L;
	}
	LIBMON_CloseNodeStruct(nNodeHdl);
	m_csDBAOwner=DBAUsernameOntarget;
	cDefNumSvr.Format(_T("%d"),m_pSvrDta->serverno);
	
	if (m_csDBAOwner.IsEmpty() ||cDefNumSvr.IsEmpty()||cDefDbName.IsEmpty())   {
		return 0L;
	}

	Cleanup();
	m_cListCtrl.DeleteAllItems();

	CString csVnodeAndUsers;
	// Read default flags on "LocalDBNode (user:XXX)"
	csVnodeAndUsers.Format(_T("%s%s%s%s"),  m_pSvrDta->LocalDBNode,
	                                    LPUSERPREFIXINNODENAME,
	                                    (LPCTSTR)m_csDBAOwner,
										LPUSERSUFFIXINNODENAME);
	nNodeHdl  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csVnodeAndUsers);
	if (nNodeHdl == -1) {
		strMsg.LoadString (IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
		strMsg1.LoadString (IDS_E_READ_FILE);    //    " - Cannot read file."
		strMsg +=  _T("\n") + strMsg1;
		MessageBox(strMsg ,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return 0L;
	}
	memset (DBAUsernameOntarget,'\0',sizeof(DBAUsernameOntarget));
	iret = DOMGetObjectLimitedInfo( nNodeHdl,
	                                parentstrings [0],
	                                (UCHAR *)"",
	                                OT_DATABASE,
	                                0,
	                                parentstrings,
	                                TRUE,
	                                &irestype,
	                                buf,
	                                DBAUsernameOntarget,
	                                extradata
	                              );
	if (iret != RES_SUCCESS) {
		LIBMON_CloseNodeStruct(nNodeHdl);
		//wsprintf((char *)buf,"DBA owner on database : %s not found. Read file aborted.",parentstrings[0]);
		strMsg.Format(IDS_F_DB_OWNER,parentstrings[0]);
		MessageBox(strMsg, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
		return 0L;
	}
	m_FlagsList.DefineAllFlagsWithDefaultValues(nNodeHdl , cDefNumSvr, cDefDbName, m_csDBAOwner);
	LIBMON_CloseNodeStruct(nNodeHdl);

	// Read Runrepl.opt from "LocalDBNode (user:XXX)" or From "RunNode"
	if (lstrcmp((char *)m_pSvrDta->LocalDBNode,(char *)m_pSvrDta->RunNode) == 0)
		csVnodeAndUsers.Format(_T("%s%s%s%s"), m_pSvrDta->LocalDBNode,
		                                   LPUSERPREFIXINNODENAME,
		                                   (LPCTSTR)m_csDBAOwner,
										   LPUSERSUFFIXINNODENAME);
	else
		csVnodeAndUsers = m_pSvrDta->RunNode;
	nNodeHdl  = LIBMON_OpenNodeStruct ((LPUCHAR)(LPCTSTR)csVnodeAndUsers);
	if (nNodeHdl == -1) {
		strMsg.LoadString(IDS_MAX_NB_CONNECT);  // _T("Maximum number of connections has been reached"
		strMsg1.LoadString (IDS_E_READ_FILE);   //    " - Cannot read file."
		strMsg += _T("\n") + strMsg1;
		MessageBox(strMsg ,NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
		return 0L;
	}

	CWaitCursor hourglass;
	// Read file Runrepl.opt and fill m_FlagsList.
	iret = GetReplicServerParams   ( pIpmDoc, m_pSvrDta, &m_FlagsList );
	
	LIBMON_CloseNodeStruct (nNodeHdl);
	CaReplicationItem* pItem = NULL;
	while (!m_FlagsList.IsEmpty())
	{
		pItem = m_FlagsList.RemoveHead();
		if (pItem->IsDisplay())
			AddItem (pItem);
		else
			delete pItem;
	}

	EnableButtons();
	return 0L;
}

LONG CuDlgReplicationServerPageStartupSetting::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationServerDataPageStartupSetting")) == 0);
	CaReplicationServerDataPageStartupSetting* pData = (CaReplicationServerDataPageStartupSetting*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0L;
	CaReplicationItemList* listItems = &(pData->m_listItems);

	try
	{
		// For each column:
		const int LAYOUT_NUMBER = 3;
		for (int i=0; i<LAYOUT_NUMBER; i++)
			m_cListCtrl.SetColumnWidth(i, pData->m_cxHeader.GetAt(i));

		CaReplicationItem* pItem = NULL;
		while (!listItems->IsEmpty())
		{
			pItem = listItems->RemoveHead();
			if (pItem->IsDisplay())
				AddItem (pItem);
			else
				delete pItem;
		}
		m_cListCtrl.SetScrollPos (SB_HORZ, pData->m_scrollPos.cx);
		m_cListCtrl.SetScrollPos (SB_VERT, pData->m_scrollPos.cy);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgReplicationServerPageStartupSetting::OnGetData (WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_WIDTH;

	CaReplicationServerDataPageStartupSetting* pData = NULL;
	try
	{
		CaReplicationItem* pItem = NULL;
		pData = new CaReplicationServerDataPageStartupSetting();
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			pItem = (CaReplicationItem*)m_cListCtrl.GetItemData(i);
			pData->m_listItems.AddTail (pItem);
		}
		//
		// For each column
		const int LAYOUT_NUMBER = 3;
		int hWidth   [LAYOUT_NUMBER] = {100, 80, 60};
		for (i=0; i<LAYOUT_NUMBER; i++)
		{
			if (m_cListCtrl.GetColumn(i, &lvcolumn))
				pData->m_cxHeader.InsertAt (i, lvcolumn.cx);
			else
				pData->m_cxHeader.InsertAt (i, hWidth[i]);
		}
		pData->m_scrollPos = CSize (m_cListCtrl.GetScrollPos (SB_HORZ), m_cListCtrl.GetScrollPos (SB_VERT));
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

LONG CuDlgReplicationServerPageStartupSetting::OnDataChanged  (WPARAM wParam, LPARAM lParam)
{
	GetDlgItem(IDC_BUTTON1)->EnableWindow (TRUE);
	return 0L;
}

LONG CuDlgReplicationServerPageStartupSetting::OnLeavingPage(WPARAM wParam, LPARAM lParam)
{
	int answ;
	if ( GetDlgItem(IDC_BUTTON1)->IsWindowEnabled() != 0 )  {
		//"Do you want to save your changes to the Startup Settings ?"
		answ = AfxMessageBox(IDS_A_STARTUP_SETTING, MB_ICONEXCLAMATION | MB_YESNO );
		if ( answ == IDYES )
			OnButtonSave();
	}
	return 0L;
}

long CuDlgReplicationServerPageStartupSetting::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}