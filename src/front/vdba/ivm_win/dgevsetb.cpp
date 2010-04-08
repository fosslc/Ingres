/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dgevsetb.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog of the bottom pane of Event Setting Frame 
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 31-Mar-2000 (uk$so01)
**    BUG 101126
**    Put the find dialog at top. When open the modal dialog, 
**    if the find dialog is opened, then close it.
** 26-Apr-2000 (uk$so01)
**    SIR SIR #101253
**    Put the "Actual Message" to be the Default radio option
** 02-Aug-2000 (uk$so01)
**    BUG #102169
**    When resize the control to become larger, make the messages to be displayed
**    in the total size of the control if posible.
** 03-Aug-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 25-Mar-2003 (uk$so01)
**    BUG #112024 / ISSUE 13214497
**    Don't call ShowMessageDescriptionFrame() on DoubleClick.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgevsetb.h"
#include "calsctrl.h"
#include "fevsetti.h"
#include "mainfrm.h"
#include "xdgevset.h"
#include "ivmdml.h"
#include "ivmsgdml.h"
#include "wmusrmsg.h"
#include "findinfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static const int GLAYOUT_NUMBER = 5;
static LSCTRLHEADERPARAMS2 lsp[GLAYOUT_NUMBER] =
{
	{IDS_HEADER_EV_TEXT,      400,  LVCFMT_LEFT, FALSE},
	{IDS_HEADER_EV_COMPONENT,  90,  LVCFMT_LEFT, FALSE},
	{IDS_HEADER_EV_NAME,       52,  LVCFMT_LEFT, FALSE},
	{IDS_HEADER_EV_INSTANCE,   84,  LVCFMT_LEFT, FALSE},
	{IDS_HEADER_EV_DATE,      170,  LVCFMT_LEFT, FALSE},
};

static void InsertSort (CTypedPtrList<CObList, CaLoggedEvent*>& le, CaLoggedEvent* e, int nStart, SORTPARAMS* sr);

static void CleanListMessage(CListCtrl* pList)
{
	int i, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		CaLoggedEvent* pItemData = (CaLoggedEvent*)pList->GetItemData (i);
		delete pItemData;
	}
	pList->DeleteAllItems();
}

static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC, n;

	SORTPARAMS* sp = (SORTPARAMS*)lParamSort;
	int nSubItem = sp->m_nItem;
	BOOL bAsc = sp->m_bAsc;

	CaLoggedEvent* lpItem1 = (CaLoggedEvent*)lParam1;
	CaLoggedEvent* lpItem2 = (CaLoggedEvent*)lParam2;

	if (lpItem1 && lpItem2)
	{
		nC = bAsc? 1 : -1;
		switch (nSubItem)
		{
		case 0:
			return nC * lpItem1->m_strText.CompareNoCase (lpItem2->m_strText);
		default:
			n = (lpItem1->GetCount() > lpItem2->GetCount())? 1: (lpItem1->GetCount() < lpItem2->GetCount())? -1: 0;
			return nC * n;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingBottom dialog


CuDlgEventSettingBottom::CuDlgEventSettingBottom(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgEventSettingBottom::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgEventSettingBottom)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nCurrentGroup = -1;
	m_sortListCtrl3.m_nItem = 1;
	m_sortListCtrl3.m_bAsc  = FALSE;
	m_sortListCtrl.m_nItem  = 0;
	m_sortListCtrl.m_bAsc   = FALSE;
	memcpy (&m_sortListCtrl2, &m_sortListCtrl, sizeof (m_sortListCtrl));

	m_bActualMessageAll  = FALSE; // Has not been queried yet
	m_bActualMessageGroup= FALSE; // Has not been queried yet
	m_cListCtrlFullDesc.SetIngresgenegicMessage();
	m_cListCtrlGroupBy.SetIngresgenegicMessage();

	m_cListCtrlFullDesc.SetCtrlType (FALSE);
	m_cListCtrlGroupBy.SetCtrlType(FALSE);
	m_cListCtrlIngres.SetCtrlType(FALSE);
}


void CuDlgEventSettingBottom::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgEventSettingBottom)
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonFind);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_cVerticalScroll);
	DDX_Control(pDX, IDC_LIST2, m_cListCtrlIngres);
	DDX_Control(pDX, IDC_LIST3, m_cListCtrlGroupBy);
	DDX_Control(pDX, IDC_COMBO1, m_cComboIngresCategory);
	DDX_Control(pDX, IDC_CHECK1, m_cGroupByMessageID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgEventSettingBottom, CDialog)
	//{{AFX_MSG_MAP(CuDlgEventSettingBottom)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_RADIO1, OnRadioActualMessage)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioIngresCategory)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboIngresCategory)
	ON_BN_CLICKED(IDC_CHECK1, OnGroupByMessageID)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST3, OnColumnclickList3)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST2, OnColumnclickList2)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonFind)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, OnDblclkList2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST3, OnDblclkList3)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, OnItemchangedList2)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST3, OnItemchangedList3)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateIcon)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()



CListCtrl* CuDlgEventSettingBottom::GetListCtrl()
{
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nRadio == IDC_RADIO1)
	{
		int nGroup = m_cGroupByMessageID.GetCheck();
		if (nGroup == 1)
			return &m_cListCtrlGroupBy;
		else
			return &m_cListCtrlFullDesc;
	}
	else
		return &m_cListCtrlIngres;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgEventSettingBottom message handlers


BOOL CuDlgEventSettingBottom::OnInitDialog() 
{
	CWaitCursor doWaitCursor;
	CDialog::OnInitDialog();
	m_cGroupByMessageID.SetCheck (1);
	m_cGroupByMessageID.EnableWindow (FALSE);
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);

	// 
	// m_cListCtrlFullDesc : Actual Message (full description)
	// m_cListCtrlGroupBy  : Actual Message (group by message ID)
	// m_cListCtrlIngres   : From ingres message category
	VERIFY (m_cListCtrlFullDesc.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create (IDB_EVENTINDICATOR_SETTINGS, 26, 0, RGB(255,0,255));
	m_cListCtrlFullDesc.SetImageList  (&m_ImageList, LVSIL_SMALL);
	m_cListCtrlIngres.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrlGroupBy.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrlFullDesc.SetCustopmVScroll(&m_cVerticalScroll);
	m_cVerticalScroll.SetListCtrl (&m_cListCtrlFullDesc);

	long lStyle = 0;
	lStyle = m_cListCtrlFullDesc.GetStyle();
	if ((lStyle & LVS_OWNERDRAWFIXED) && !m_cListCtrlFullDesc.IsOwnerDraw())
	{
		ASSERT (FALSE);
		return FALSE;
	}
	lStyle = m_cListCtrlIngres.GetStyle();
	if ((lStyle & LVS_OWNERDRAWFIXED) && !m_cListCtrlIngres.IsOwnerDraw())
	{
		ASSERT (FALSE);
		return FALSE;
	}
	lStyle = m_cListCtrlGroupBy.GetStyle();
	if ((lStyle & LVS_OWNERDRAWFIXED) && !m_cListCtrlGroupBy.IsOwnerDraw())
	{
		ASSERT (FALSE);
		return FALSE;
	}

	LV_COLUMN lvcolumn;

	//
	// Actual messages from Logged events:
	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	for (int i=0; i<GLAYOUT_NUMBER; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrlFullDesc.InsertColumn (i, &lvcolumn);

		if (i==0)
		{
			//
			// Ingres Messages:
			lvcolumn.cx = 400;
			m_cListCtrlGroupBy.InsertColumn (i, &lvcolumn);
			m_cListCtrlIngres.InsertColumn (i, &lvcolumn);
		}
		if (i == 1)
		{
			strHeader.LoadString (IDS_HEADER_STAT_COUNT);
			lvcolumn.fmt      = LVCFMT_RIGHT;
			lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
			m_cListCtrlGroupBy.InsertColumn (i, &lvcolumn);
		}
	}
	ResizeControls();

	try
	{
		//
		// Fill the Combobox of Ingres Category:
		FillIngresCategory();
		int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
		switch (nRadio)
		{
		case IDC_RADIO1:
			OnRadioActualMessage();
			break;
		case IDC_RADIO2:
			OnRadioIngresCategory();
			break;
		default:
			ASSERT (FALSE);
			break;
		}
		EnableButtonFind();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuDlgEventSettingBottom::OnInitDialog): \nCannot initialize messages."));
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CuDlgEventSettingBottom::FillIngresCategory()
{
	CaMessageEntry* pEntry = NULL;
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;

	CaMessageManager* pMsgManager = pFrame->GetMessageManager();
	ASSERT (pMsgManager);
	if (!pMsgManager)
		return;
	CTypedPtrArray <CObArray, CaMessageEntry*>& aEntry = pMsgManager->GetListEntry();
	int idx;
	INT_PTR nSize = aEntry.GetSize();
	
	for (int i=0; i<nSize; i++)
	{
		pEntry = aEntry [i];
		idx = m_cComboIngresCategory.AddString (pEntry->GetIngresCategoryTitle());
		if (idx != CB_ERR)
		{
			m_cComboIngresCategory.SetItemData (idx, (DWORD_PTR)pEntry);
		}
	}
}


void CuDlgEventSettingBottom::ResizeControls()
{
	CWnd* pCtrl = NULL;
	CRect rDlg, r, rScroll;
	if (!IsWindow (m_cVerticalScroll.m_hWnd))
		return;

	int nGroup = m_cGroupByMessageID.GetCheck();
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nRadio == IDC_RADIO1)
	{
		m_cListCtrlIngres.ShowWindow (SW_HIDE);
		if (nGroup == 1)
		{
			m_cListCtrlFullDesc.ShowWindow (SW_HIDE);
			m_cListCtrlGroupBy.ShowWindow (SW_SHOW);
			pCtrl = &m_cListCtrlGroupBy;
		}
		else
		{
			m_cListCtrlFullDesc.ShowWindow (SW_SHOW);
			m_cListCtrlGroupBy.ShowWindow (SW_HIDE);
			pCtrl = &m_cListCtrlFullDesc;
		}
	}
	else
	{
		m_cListCtrlIngres.ShowWindow (SW_SHOW);
		m_cListCtrlFullDesc.ShowWindow (SW_HIDE);
		m_cListCtrlGroupBy.ShowWindow (SW_HIDE);
		pCtrl = &m_cListCtrlIngres;
	}
	if (!pCtrl)
		return;

	GetClientRect (rDlg);
	pCtrl->GetWindowRect (r);
	ScreenToClient (r);
	r.left  = rDlg.left  + 1;
	r.right = rDlg.right - 1;
	r.bottom= rDlg.bottom- 1;
	
	if (nGroup != 1 && nRadio == IDC_RADIO1 && m_cListCtrlFullDesc.IsListCtrlScrollManagement())
	{
		m_cVerticalScroll.GetWindowRect (rScroll);
		r.right -= rScroll.Width();
	}
	pCtrl->MoveWindow (r);

	if (nGroup != 1 && nRadio == IDC_RADIO1 && m_cListCtrlFullDesc.IsListCtrlScrollManagement())
	{
		r.right = rDlg.right - 1;
		r.left   = r.right - rScroll.Width();
		m_cVerticalScroll.MoveWindow (r);
		m_cVerticalScroll.ShowWindow (SW_SHOW);
	}
	else
		m_cVerticalScroll.ShowWindow (SW_HIDE);

	if (m_cListCtrlFullDesc.IsListCtrlScrollManagement())
	{
		//
		// Array of events of the current pane:
		CObArray& arrayEvent = m_cListCtrlFullDesc.GetArrayEvent();
		int nItemCount = m_cListCtrlFullDesc.GetItemCount();
		int nCountPerPage = m_cListCtrlFullDesc.GetCountPerPage();
		int nPos = m_cVerticalScroll.GetScrollPosExtend();
		INT_PTR nTop = m_cListCtrlFullDesc.GetTopIndex();
		INT_PTR nUpperBound = arrayEvent.GetUpperBound();
		BOOL bLastItemVisible = FALSE;
		if (nItemCount > 0 && nUpperBound != -1 && nTop != -1 && (nTop + nCountPerPage - 1) >= 0)
		{
			int nLastItem = (int)(nTop + nCountPerPage - 1);
			if (nItemCount < nCountPerPage)
				nLastItem = nItemCount - 1;
			CaLoggedEvent* pLastVisible = (CaLoggedEvent*)m_cListCtrlFullDesc.GetItemData(nLastItem);
			CaLoggedEvent* pLastItem    = (CaLoggedEvent*)arrayEvent.GetAt(nUpperBound);
			if (pLastVisible == pLastItem)
				bLastItemVisible = TRUE;
		}

		if (bLastItemVisible)
		{
			CaLoggedEvent* pEvSelected = NULL;
			int nSelectedEvent = m_cListCtrlFullDesc.GetNextItem (-1, LVNI_SELECTED);
			if (nSelectedEvent != -1)
			{
				pEvSelected = (CaLoggedEvent*)m_cListCtrlFullDesc.GetItemData(nSelectedEvent);
			}

			if (nUpperBound+1 <= nCountPerPage)
			{
				nPos = 0;
				m_cListCtrlFullDesc.MoveAt(nPos);
				m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos);
				m_cVerticalScroll.EnableWindow(FALSE);
			}
			else
			{
				m_cVerticalScroll.EnableWindow(TRUE);
				nPos = (int)(nUpperBound - nCountPerPage + 1);
				if (nPos >= 0 && nPos <= nUpperBound)
				{
					m_cListCtrlFullDesc.MoveAt(nPos);
					m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos);
				}
			}
			//
			// Reselect the previous selected item if it is still there:
			if (pEvSelected != NULL)
			{
				int nItemCount = m_cListCtrlFullDesc.GetItemCount();
				for (int i=0; i<nItemCount; i++)
				{
					if (pEvSelected == (CaLoggedEvent*)m_cListCtrlFullDesc.GetItemData(i))
					{
						m_cListCtrlFullDesc.SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						break;
					}
				}
			}
		}
		//
		// Enable/Disable scrollbar:
		nItemCount = m_cListCtrlFullDesc.GetItemCount();
		if (nItemCount <= nCountPerPage)
			m_cVerticalScroll.EnableWindow (FALSE);
		else
			m_cVerticalScroll.EnableWindow (TRUE);
	}
}


void CuDlgEventSettingBottom::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrlFullDesc.m_hWnd))
		return;

	ResizeControls();
}

void CuDlgEventSettingBottom::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgEventSettingBottom::HandleActualMessage()
{
	CWaitCursor doWaitCursor;

	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	CaMessageEntry* pEntry = NULL;
	CaMessageManager* pMessageManager = pFrame->GetMessageManager();
	if (!pMessageManager)
		return;

	ResizeControls();
	EnableControls();
	int nGroup = m_cGroupByMessageID.GetCheck();
	if (m_nCurrentGroup == nGroup)
		return;
	m_nCurrentGroup = nGroup;

	int nIndex;
	Imsgclass nClass;
	CaLoggedEvent* pExistMessage = NULL;
	CaMessage* pMsg;
	CaLoggedEvent* pEv;
	CaIvmEvent& loggedEvent = theApp.GetEventData();
	CTypedPtrList<CObList, CaLoggedEvent*>& le = loggedEvent.Get();
	POSITION pos = le.GetHeadPosition();
	
	//
	// Group by:
	BOOL bGroupBy = FALSE;
	if (m_nCurrentGroup == 1) 
	{
		bGroupBy = TRUE;
		//
		// Messages have been queried and ready to be used:
		if (m_bActualMessageGroup)
			return;
		//
		// Initialize the Group Message:
		m_bActualMessageGroup = TRUE;
		m_cListCtrlGroupBy.LockWindowUpdate();
		
		while (pos != NULL)
		{
			pEv = le.GetNext (pos);
			pEv->SetClassify(FALSE);
			//
			// Check to see if the list of Group By Event containts already
			// an event whose m_lCode = pEv->m_lCode.
			// If exist, just increase the count number !!
			pExistMessage = ExistMessage (pEv, nIndex);
			if (pExistMessage)
			{
				int iCount,iCountExtra;
				CString strCount;
				iCount = pExistMessage->GetCount();
				iCountExtra = pExistMessage->GetExtraCount();
				if (pEv->IsNotFirstLine()) {
					iCountExtra++;
					pExistMessage->SetExtraCount(iCountExtra);
				}
				else {
					iCount++;
					pExistMessage->SetCount(iCount);
				}
				if (iCountExtra == 0)
					strCount.Format (_T("%d"),iCount);
				else
					strCount.Format (_T("%d(+%d)"),iCount,iCountExtra);
				m_cListCtrlGroupBy.SetItemText (nIndex, 1, strCount);
				continue;
			}
			
			//
			// The event has not been yet added to the list of Group By Event.
			// Mark this event as classified or not due to the Entry of Message Manager:
			pEntry = pMessageManager->FindEntry (pEv->GetCatType());
			ASSERT (pEntry);
			if (!pEntry)
				continue;

			pMsg = pEntry->Search(pEv->GetCode());
			nClass = pEv->GetClass();
		
			if (pMsg)
				pEv->SetClass (pMsg->GetClass());
			else
				pEv->SetClass (pEntry->GetClass());
			pEv->SetClassify(pMsg? TRUE: FALSE);
			if (pEv->IsNotFirstLine()) {
				pEv->SetCount(0);
				pEv->SetExtraCount(1);
			}
			else {
				pEv->SetCount(1);
				pEv->SetExtraCount(0);
			}
			AddEventToGroupByList(pEv);
		}
		m_cListCtrlGroupBy.SortItems(CompareSubItem, (LPARAM)&m_sortListCtrl3);
		m_cListCtrlGroupBy.UnlockWindowUpdate();
	}

	//
	// Full description:
	if (m_nCurrentGroup != 1)
	{
		//
		// Messages have been queried and ready to be used:
		if (m_bActualMessageAll)
			return;
		//
		// Initialize the Full Description Message:
		m_bActualMessageAll = TRUE;
		if (!m_listLoggedEvent.IsEmpty())
			m_listLoggedEvent.RemoveAll();
		loggedEvent.GetAll (m_listLoggedEvent);
		INT_PTR nCount = m_listLoggedEvent.GetCount();
		pos = m_listLoggedEvent.GetHeadPosition();
		
		while (pos != NULL)
		{
			pEv = m_listLoggedEvent.GetNext (pos);
			pEv->SetClassify(FALSE);
			//
			// The event has not been yet added to the list of Group By Event.
			// Mark this event as classified or not due to the Entry of Message Manager:
			pEntry = pMessageManager->FindEntry (pEv->GetCatType());
			ASSERT (pEntry);
			if (!pEntry)
				continue;

			pMsg = pEntry->Search(pEv->GetCode());
			nClass = pEv->GetClass();
		
			if (pMsg)
				pEv->SetClass (pMsg->GetClass());
			else
				pEv->SetClass (pEntry->GetClass());
			pEv->SetClassify(pMsg? TRUE: FALSE);
			
		}

		int nCountPerPage = m_cListCtrlFullDesc.GetCountPerPage();
		if (nCount < theApp.GetScrollManagementLimit())
		{
			m_cListCtrlFullDesc.SetListCtrlScrollManagement(FALSE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}
		else
		{
			m_cListCtrlFullDesc.SetListCtrlScrollManagement(TRUE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}
		ResizeControls();
		EnableControls();

		m_cVerticalScroll.SetListCtrl    (&m_cListCtrlFullDesc);
		m_cVerticalScroll.SetScrollRange (0, (int)(nCount - nCountPerPage));
		m_cVerticalScroll.SetScrollPos   (0);
		m_cListCtrlFullDesc.SetListEvent(&m_listLoggedEvent);
		m_cListCtrlFullDesc.InitializeItems (GLAYOUT_NUMBER);
	}
	EnableButtonFind();
}

void CuDlgEventSettingBottom::OnRadioActualMessage() 
{
	HandleActualMessage();
	if (m_nCurrentGroup == 1)
		ShowMessageDescriptionFrame(NULL, &m_cListCtrlGroupBy, FALSE, TRUE);
	else
		ShowMessageDescriptionFrame(NULL, &m_cListCtrlFullDesc, FALSE, TRUE);
}

void CuDlgEventSettingBottom::OnRadioIngresCategory() 
{
	ResizeControls();
	EnableControls();
	EnableButtonFind();
	ShowMessageDescriptionFrame(NULL, &m_cListCtrlIngres, FALSE, TRUE);
}

void CuDlgEventSettingBottom::OnSelchangeComboIngresCategory() 
{
	CWaitCursor doWaitcursor;
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nRadio != IDC_RADIO2)
		return;
	int nSel = m_cComboIngresCategory.GetCurSel();
	if (nSel == CB_ERR)
		return;
	CaMessageEntry* pEntry = (CaMessageEntry*)m_cComboIngresCategory.GetItemData (nSel);
	if (!pEntry)
		return;

	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	CaMessageManager* pMessageManager = pFrame->GetMessageManager();
	if (!pMessageManager)
		return;
	
	CListCtrl* pListCtrl = &m_cListCtrlIngres;
	CTypedPtrList<CObList, CaLoggedEvent*> lsMsg;
	BOOL bOk = IVM_QueryIngresMessage (pEntry, lsMsg);

	if (!(bOk && pListCtrl))
		return;

	CleanListMessage(pListCtrl);
	CaMessage* pMsg = NULL;
	POSITION pos = lsMsg.GetHeadPosition();
	if (lsMsg.GetCount() > 100)
		pListCtrl->SetItemCount ((int)lsMsg.GetCount());
	while (!lsMsg.IsEmpty())
	{
		CaLoggedEvent* pObj = lsMsg.RemoveHead();
		pMsg = IVM_LookupMessage (pObj->GetCatType(), pObj->GetCode(), pMessageManager);
		if (pMsg)
			pObj->SetClass (pMsg->GetClass());
		else
			pObj->SetClass (pEntry->GetClass());
		pObj->SetClassify(pMsg? TRUE: FALSE);
		AddEventToIngresMessage (pObj);
	}
	EnableButtonFind();
}

void CuDlgEventSettingBottom::OnGroupByMessageID() 
{
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	//
	// If ingres message category then do nothing
	if (nRadio == IDC_RADIO2)
		return;
	OnRadioActualMessage();
	if (m_nCurrentGroup == 1)
		ShowMessageDescriptionFrame(NULL, &m_cListCtrlGroupBy, FALSE, TRUE);
	else
		ShowMessageDescriptionFrame(NULL, &m_cListCtrlFullDesc, FALSE, TRUE);
	ResizeControls();
}

CaLoggedEvent* CuDlgEventSettingBottom::ExistMessage (CaLoggedEvent* pEv, int& nIndex)
{
	int i, nCount = m_cListCtrlGroupBy.GetItemCount();
	CaLoggedEvent* pItemData = NULL;
	for (i=0; i<nCount; i++)
	{
		pItemData = (CaLoggedEvent*)m_cListCtrlGroupBy.GetItemData(i);
		if (pItemData->GetCode() == pEv->GetCode())
		{
			nIndex = i;
			return pItemData;
		}

	}
	return NULL;
}


void CuDlgEventSettingBottom::AddEvent (CaLoggedEvent* pEvent, int nCtrl)
{
	int  nImage = 0;
	int  nIndex = -1;
	int nGroup = m_cGroupByMessageID.GetCheck();

	UINT nState = 0;
	BOOL bClassified = pEvent->IsClassified();
	CListCtrl* pCtrl = GetListCtrl();

	int nCount = pCtrl->GetItemCount();
	Imsgclass msgClass = pEvent->GetClass();

	switch (msgClass)
	{
	case IMSG_ALERT:
		nImage  = bClassified? IM_ALERT: IM_ALERT_U;
		nState |= bClassified? (M_ALERT|M_CLASSIFIED): M_ALERT;
		break;
	case IMSG_NOTIFY:
		nImage  = bClassified? IM_NOTIFY: IM_NOTIFY_U;
		nState |= bClassified? (M_NOTIFY|M_CLASSIFIED): M_NOTIFY;
		break;
	case IMSG_DISCARD:
		nImage  = bClassified? IM_DISCARD: IM_DISCARD_U;
		nState |= bClassified? (M_DISCARD|M_CLASSIFIED): M_DISCARD;
		break;
	}
	
	CaMessage* pMsg = NULL;
	CaMessageItemData* pItemData = NULL;
	pItemData = new CaMessageItemData(pEvent->m_strText, pEvent->GetCatType(), pEvent->GetCode(), nState);
	CString strText = pEvent->m_strText;
	if (nCtrl == 3 && nGroup == 1)
	{
		strText = IVM_IngresGenericMessage (pEvent->GetCode(), pEvent->m_strText);
	}

	nIndex = pCtrl->InsertItem  (nCount, strText, nImage);
	if (nIndex != -1)
	{
		if (nCtrl == 2 || (nCtrl == 1 && nGroup != 1))
		{
			pCtrl->SetItemText (nIndex, 1, pEvent->m_strComponentType);
			pCtrl->SetItemText (nIndex, 2, pEvent->m_strComponentName);
			pCtrl->SetItemText (nIndex, 3, pEvent->m_strInstance);
			pCtrl->SetItemText (nIndex, 4, pEvent->GetDateLocale());
		}
		else
		{
			TCHAR tchszText [16];
			wsprintf (tchszText, _T("%d"), pItemData->GetCount());
			pCtrl->SetItemText (nIndex, 1, tchszText);
		}
		pCtrl->SetItemData (nIndex, (DWORD_PTR)pItemData);
	}
	else
		delete pItemData;
}


void CuDlgEventSettingBottom::OnDestroy() 
{
	
	CfMainFrame* pFrame = (CfMainFrame* )AfxGetMainWnd();
	if (pFrame)
		pFrame->Search(NULL);

	CleanListMessage (&m_cListCtrlIngres);
	CDialog::OnDestroy();
}

void CuDlgEventSettingBottom::EnableControls()
{
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nRadio != IDC_RADIO2)
	{
		m_cComboIngresCategory.EnableWindow (FALSE);
		m_cGroupByMessageID.EnableWindow(TRUE);
	}
	else
	{
		m_cComboIngresCategory.EnableWindow (TRUE);
		m_cGroupByMessageID.EnableWindow(FALSE);
	}
}

void CuDlgEventSettingBottom::ClassifyMessages(long lCat, long lCode, BOOL bActualMessage)
{
	int  nImage = 0;
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	CaMessageManager* pMessageManager = pFrame->GetMessageManager();
	if (!pMessageManager)
		return;


	Imsgclass nc = IMSG_NOTIFY;
	BOOL bAll = (lCat == -1)? TRUE: FALSE;
	LV_ITEM item;
	memset (&item, 0, sizeof(item));
	item.mask = LVIF_IMAGE;
	CaLoggedEvent* pMsg = NULL;
	CaMessage* pMsgState = NULL;

	BOOL bLookUp = FALSE;
	CaIvmEvent& loggedEvent = theApp.GetEventData();
	CTypedPtrList<CObList, CaLoggedEvent*>& le = loggedEvent.Get();
	POSITION pos = le.GetHeadPosition();
	while (pos != NULL)
	{
		CaLoggedEvent* pEv = le.GetNext (pos);
		if (!bAll)
		{
			if (pEv->GetCatType() != lCat)
				continue;
			if (pEv->GetCode() != lCode)
				continue;

			if (!bLookUp)
			{
				pMsgState = IVM_LookupMessage (lCat, lCode, pMessageManager);
				bLookUp = TRUE;
			}
		}
		else
		{
			pMsgState = IVM_LookupMessage (pEv->GetCatType(), pEv->GetCode(), pMessageManager);
		}

		if (!pMsgState)
		{
			pEv->SetClassify (FALSE);
			pEv->SetClass (IMSG_NOTIFY);
		}
		else
		{
			pEv->SetClassify (TRUE);
			pEv->SetClass (pMsgState->GetClass());
		}
	}

	int i, nCount = m_cListCtrlGroupBy.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pMsg = (CaLoggedEvent*)m_cListCtrlGroupBy.GetItemData (i);
		if (!bAll)
		{
			if (pMsg->GetCatType() != lCat)
				continue;
			if (pMsg->GetCode() != lCode)
				continue;
		}

		item.iItem = i;
		nc = pMsg->GetClass();
		switch (nc)
		{
		case IMSG_ALERT:
			item.iImage = pMsg->IsClassified()? IM_ALERT: IM_ALERT_U;
			break;
		case IMSG_NOTIFY:
			item.iImage = pMsg->IsClassified()? IM_NOTIFY: IM_NOTIFY_U;
			break;
		case IMSG_DISCARD:
			item.iImage = pMsg->IsClassified()? IM_DISCARD: IM_DISCARD_U;
			break;
		}
		m_cListCtrlGroupBy.SetItem (&item);
	}

	nCount = m_cListCtrlFullDesc.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pMsg = (CaLoggedEvent*)m_cListCtrlFullDesc.GetItemData (i);
		if (!bAll)
		{
			if (pMsg->GetCatType() != lCat)
				continue;
			if (pMsg->GetCode() != lCode)
				continue;
		}

		item.iItem = i;
		nc = pMsg->GetClass();
		switch (nc)
		{
		case IMSG_ALERT:
			item.iImage = pMsg->IsClassified()? IM_ALERT: IM_ALERT_U;
			break;
		case IMSG_NOTIFY:
			item.iImage = pMsg->IsClassified()? IM_NOTIFY: IM_NOTIFY_U;
			break;
		case IMSG_DISCARD:
			item.iImage = pMsg->IsClassified()? IM_DISCARD: IM_DISCARD_U;
			break;
		}
		m_cListCtrlFullDesc.SetItem (&item);
	}
}


void CuDlgEventSettingBottom::ClassifyIngresMessages(long lCat, long lCode)
{
	int  nImage = 0;

	CListCtrl* pCtrl = &m_cListCtrlIngres;
	if (!pCtrl)
		return;
	CfEventSetting* pFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return;
	CaMessageManager* pMessageManager = pFrame->GetMessageManager();
	if (!pMessageManager)
		return;


	Imsgclass nc = IMSG_NOTIFY;
	BOOL bAll = (lCat == -1)? TRUE: FALSE;
	LV_ITEM item;
	memset (&item, 0, sizeof(item));
	item.mask = LVIF_IMAGE;
	CaLoggedEvent* pMsg = NULL;
	CaMessage* pMsgState = NULL;
	int i, nCount = pCtrl->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pMsg = (CaLoggedEvent*)pCtrl->GetItemData (i);
		if (!bAll)
		{
			if (pMsg->GetCatType() != lCat)
				continue;
			if (pMsg->GetCode() != lCode)
				continue;
		}

		item.iItem = i;
		pMsgState = IVM_LookupMessage (pMsg->GetCatType(), pMsg->GetCode(), pMessageManager);
		if (!pMsgState)
		{
			pMsg->SetClassify(FALSE);
			nc = IMSG_NOTIFY;
		}
		else
		{
			pMsg->SetClassify(TRUE);
			nc = pMsgState->GetClass();
		}
			
		pMsg->SetClass(nc);
		switch (nc)
		{
		case IMSG_ALERT:
			item.iImage = pMsg->IsClassified()? IM_ALERT: IM_ALERT_U;
			break;
		case IMSG_NOTIFY:
			item.iImage = pMsg->IsClassified()? IM_NOTIFY: IM_NOTIFY_U;
			break;
		case IMSG_DISCARD:
			item.iImage = pMsg->IsClassified()? IM_DISCARD: IM_DISCARD_U;
			break;
		}
		
		pCtrl->SetItem (&item);
	}
}


LONG CuDlgEventSettingBottom::OnUpdateIcon (WPARAM w, LPARAM l)
{
	CWaitCursor doWaitCursor;
	int nCat, nCode;
	nCat = (long)w;
	nCode= (long)l;
	ClassifyMessages(nCat, nCode, TRUE);
	ClassifyIngresMessages(nCat, nCode);
	return 0;
}

//
// Group By Message ID:
void CuDlgEventSettingBottom::OnColumnclickList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SORTPARAMS sp;
	sp.m_nItem = pNMListView->iSubItem;
	if (m_sortListCtrl3.m_nItem == sp.m_nItem)
		sp.m_bAsc = !m_sortListCtrl3.m_bAsc;
	else
		sp.m_bAsc = TRUE;
	memcpy (&m_sortListCtrl3, &sp, sizeof(m_sortListCtrl3));
	m_cListCtrlGroupBy.SortItems(CompareSubItem, (LPARAM)&sp);
	
	*pResult = 0;
}

//
// Full Description:
void CuDlgEventSettingBottom::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SORTPARAMS sp;
	if (pNMListView->iSubItem == 0)
	{
		sp.m_nItem = pNMListView->iSubItem;
		if (m_sortListCtrl.m_nItem == sp.m_nItem)
			sp.m_bAsc = !m_sortListCtrl.m_bAsc;
		else
			sp.m_bAsc = TRUE;
		memcpy (&m_sortListCtrl, &sp, sizeof(m_sortListCtrl3));


		CObArray& arrayEvent = m_cListCtrlFullDesc.GetArrayEvent();
		CProgressCtrl cProgress;
		CfEventSetting* pFrm = (CfEventSetting*)GetParentFrame();

		if (pFrm)
		{
			CxDlgEventSetting* pDlg = (CxDlgEventSetting*)pFrm->GetParent();
			if (pDlg && IsWindow (pDlg->m_cStaticProgress.m_hWnd))
			{
				CRect rc;
				pDlg->m_cStaticProgress.GetWindowRect(rc);
				pDlg->ScreenToClient(rc);
				VERIFY (cProgress.Create(WS_CHILD | WS_VISIBLE, rc, pDlg, 1));
			}
		}

		IVM_DichotomySort(arrayEvent, CompareSubItem, (LPARAM)&m_sortListCtrl, &cProgress);
		m_cListCtrlFullDesc.Sort ((LPARAM)&m_sortListCtrl, CompareSubItem);
	}

	*pResult = 0;
}

//
// Ingres Category:
void CuDlgEventSettingBottom::OnColumnclickList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SORTPARAMS sp;
	if (pNMListView->iSubItem == 0)
	{
		sp.m_nItem = pNMListView->iSubItem;
		if (m_sortListCtrl2.m_nItem == sp.m_nItem)
			sp.m_bAsc = !m_sortListCtrl2.m_bAsc;
		else
			sp.m_bAsc = TRUE;
		memcpy (&m_sortListCtrl2, &sp, sizeof(m_sortListCtrl2));
		m_cListCtrlIngres.SortItems(CompareSubItem, (LPARAM)&sp);
	}

	*pResult = 0;
}

void CuDlgEventSettingBottom::AddEventToGroupByList(CaLoggedEvent* pEv)
{
	int  nImage = 0;
	int  nIndex = -1;

	BOOL bClassified = pEv->IsClassified();
	CListCtrl* pCtrl = &m_cListCtrlGroupBy;

	int nCount = pCtrl->GetItemCount();
	Imsgclass msgClass = pEv->GetClass();

	switch (msgClass)
	{
	case IMSG_ALERT:
		nImage  = bClassified? IM_ALERT: IM_ALERT_U;
		break;
	case IMSG_NOTIFY:
		nImage  = bClassified? IM_NOTIFY: IM_NOTIFY_U;
		break;
	case IMSG_DISCARD:
		nImage  = bClassified? IM_DISCARD: IM_DISCARD_U;
		break;
	}
	
	CString strText = IVM_IngresGenericMessage (pEv->GetCode(), pEv->m_strText);
	nIndex = pCtrl->InsertItem  (nCount, strText, nImage);
	if (nIndex != -1)
	{
		TCHAR tchszText [32];
		if (pEv->IsNotFirstLine())
			_tcscpy(tchszText, _T("0(+1)"));
		else
			_tcscpy(tchszText, _T("1"));
		pCtrl->SetItemText (nIndex, 1, tchszText);
		pCtrl->SetItemData (nIndex, (DWORD_PTR)pEv);
	}
}


void CuDlgEventSettingBottom::AddEventToFullDescList(CaLoggedEvent* pEv)
{
	int  nImage = 0;
	int  nIndex = -1;

	BOOL bClassified = pEv->IsClassified();
	CListCtrl* pCtrl = GetListCtrl();

	int nCount = pCtrl->GetItemCount();
	Imsgclass msgClass = pEv->GetClass();

	switch (msgClass)
	{
	case IMSG_ALERT:
		nImage  = bClassified? IM_ALERT: IM_ALERT_U;
		break;
	case IMSG_NOTIFY:
		nImage  = bClassified? IM_NOTIFY: IM_NOTIFY_U;
		break;
	case IMSG_DISCARD:
		nImage  = bClassified? IM_DISCARD: IM_DISCARD_U;
		break;
	}
	
	nIndex = pCtrl->InsertItem  (nCount, pEv->m_strText, nImage);
	if (nIndex != -1)
	{
		pCtrl->SetItemText (nIndex, 1, pEv->m_strComponentType);
		pCtrl->SetItemText (nIndex, 2, pEv->m_strComponentName);
		pCtrl->SetItemText (nIndex, 3, pEv->m_strInstance);
		pCtrl->SetItemText (nIndex, 4, pEv->GetDateLocale());
		pCtrl->SetItemData (nIndex, (DWORD_PTR)pEv);
	}
}

void CuDlgEventSettingBottom::AddEventToIngresMessage(CaLoggedEvent* pEv)
{
	int  nImage = 0;
	int  nIndex = -1;

	BOOL bClassified = pEv->IsClassified();
	CListCtrl* pCtrl = &m_cListCtrlIngres;

	int nCount = pCtrl->GetItemCount();
	Imsgclass msgClass = pEv->GetClass();

	switch (msgClass)
	{
	case IMSG_ALERT:
		nImage  = bClassified? IM_ALERT: IM_ALERT_U;
		break;
	case IMSG_NOTIFY:
		nImage  = bClassified? IM_NOTIFY: IM_NOTIFY_U;
		break;
	case IMSG_DISCARD:
		nImage  = bClassified? IM_DISCARD: IM_DISCARD_U;
		break;
	}
	
	nIndex = pCtrl->InsertItem  (nCount, pEv->m_strText, nImage);
	if (nIndex != -1)
	{
		pCtrl->SetItemData (nIndex, (DWORD_PTR)pEv);
	}
}


void CuDlgEventSettingBottom::SortByInsert(LPARAM l)
{
	// NOT IMPLEMENTED
	ASSERT (FALSE);
}

static void InsertSort (CTypedPtrList<CObList, CaLoggedEvent*>& le, CaLoggedEvent* e, int nStart, SORTPARAMS* sr)
{
	int nCmp = 0;
	int nSort= 1; // Range of element to compare.
	BOOL bContinue = TRUE;
	CaLoggedEvent* elm = NULL;
	POSITION p, pos = le.GetHeadPosition();
	while (pos != NULL && bContinue)
	{
		p = pos;
		elm = le.GetNext (pos);
		if (nSort >= nStart || pos == NULL)
			bContinue = FALSE;
		nCmp = CompareSubItem ((LPARAM)e, (LPARAM)elm, (LPARAM)sr);
		if (nCmp >= 0 && bContinue)
		{
			nSort++;
			continue;
		}
		if (nCmp >= 0)
		{
			//
			// e >= elm: insert e after elm.
			le.InsertAfter (p, e);
		}
		else
		{
			//
			// e < elm: insert e before elm.
			le.InsertBefore (p, e);
		}
		break;
	}
}

void CuDlgEventSettingBottom::OnButtonFind() 
{
	CListCtrl* pListCtrl = NULL;
	int nGroup = m_cGroupByMessageID.GetCheck();
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);

	if (nRadio == IDC_RADIO1)
		pListCtrl = (nGroup == 1)? &m_cListCtrlGroupBy: &m_cListCtrlFullDesc;
	else
		pListCtrl = &m_cListCtrlIngres;

	if (pListCtrl && IsWindow (pListCtrl->m_hWnd))
	{
		CfMainFrame* pFrame = (CfMainFrame* )AfxGetMainWnd();
		if (pFrame)
			pFrame->Search(this);
	}
}

void CuDlgEventSettingBottom::EnableButtonFind()
{
	BOOL bEnable = FALSE;
	CListCtrl* pListCtrl = NULL;
	int nGroup = m_cGroupByMessageID.GetCheck();
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);

	if (nRadio == IDC_RADIO1)
		pListCtrl = (nGroup == 1)? &m_cListCtrlGroupBy: &m_cListCtrlFullDesc;
	else
		pListCtrl = &m_cListCtrlIngres;

	if (pListCtrl && IsWindow (pListCtrl->m_hWnd))
	{
		if (pListCtrl->GetItemCount() > 0)
			bEnable = TRUE;
	}
	m_cButtonFind.EnableWindow (bEnable);
}


LONG CuDlgEventSettingBottom::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgEventSettingBottom::OnFind \n");
	CuListCtrlLoggedEvent* pListCtrl = NULL;

	int nGroup = m_cGroupByMessageID.GetCheck();
	int nRadio = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	if (nRadio == IDC_RADIO1)
		pListCtrl = (nGroup == 1)? &m_cListCtrlGroupBy: &m_cListCtrlFullDesc;
	else
		pListCtrl = &m_cListCtrlIngres;
	if (pListCtrl)
		return pListCtrl->FindText (wParam, lParam);

	return 0L;
}

void CuDlgEventSettingBottom::ShowMessageDescriptionFrame(NMLISTVIEW* pNMListView, CuListCtrlLoggedEvent* pCtrl, BOOL bCreate, BOOL bUpdate)
{
	if (!pCtrl)
		return;
	if (!IsWindow(pCtrl->m_hWnd))
		return;
	int nPos = -1;
	if (bUpdate)
	{
		int i, nCount = pCtrl->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (pCtrl->GetItemState (i, LVIS_SELECTED)&LVIS_SELECTED)
			{
				nPos = i;
				break;
			}
		}
	}
	else
	{
		if (pNMListView)
			nPos = pNMListView->iItem;
	}


	if (nPos >= 0 && (pCtrl->GetItemState (nPos, LVIS_SELECTED)&LVIS_SELECTED))
	{
		CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
		if (pFmain)
		{
			CaLoggedEvent* pEvent = (CaLoggedEvent*)pCtrl->GetItemData(nPos);
			if (!pEvent)
				return;
			MSGCLASSANDID msg;
			msg.lMsgClass = pEvent->GetCatType();
			msg.lMsgFullID= pEvent->GetCode();
			pFmain->ShowMessageDescriptionFrame(bCreate, &msg);
		}
	}
}

void CuDlgEventSettingBottom::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	//ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlFullDesc, TRUE);
	*pResult = 0;
}

void CuDlgEventSettingBottom::OnDblclkList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	//ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlIngres, TRUE);
	*pResult = 0;
}

void CuDlgEventSettingBottom::OnDblclkList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	//ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlGroupBy, TRUE);
	*pResult = 0;
}

void CuDlgEventSettingBottom::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlFullDesc, FALSE);
	}

	*pResult = 0;
}

void CuDlgEventSettingBottom::OnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlIngres, FALSE);
	}

	*pResult = 0;
}

void CuDlgEventSettingBottom::OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		ShowMessageDescriptionFrame(pNMListView, &m_cListCtrlGroupBy, FALSE);
	}

	*pResult = 0;
}
