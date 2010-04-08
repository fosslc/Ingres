/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : udlgmain.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Main dialog (modeless) of VCDA.OCX
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 21-Jan-2004 (uk$so01)
**    SIR #109221, Remove the leading and trailing space.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 03-Oct-2004 (schph01)
**    BUG #113371, search the parenthesis to determine the begining of the
**    platform type , instead of the fix value.
** 17-May-2005 (komve01)
**    Issue# 13864404, Bug#114371. 
**    Added a parameter for making the list control a simple list control.
**    The default behavior is the same as earlier, but for a special case
**    like VCDA (Main Parameters Tab), we do not want the default behavior
**    rather we would just like to have it as a simple list control.
** 10-May-2005 (lakvi01/komve01)
**	    When comparing for platform (during restore), do not compare build 
**	    number, just compare "(platform.id".
*/

#include "stdafx.h"
#include <htmlhelp.h>
#include "vcda.h"
#include "udlgmain.h"
#include "pagediff.h"
#include "rctools.h"
#include "xmaphost.h"
#include "wmusrmsg.h"
#include "pshresto.h"
#include "vcdactl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 4
#if defined (_VIRTUAL_NODE_AVAILABLE)
#define OTHER_HOST_INDEX 4 // (0: config, 1: parameter (system), 2: parameter (user), 3: vnode, 4: other hosts.
#else
#define OTHER_HOST_INDEX 3 // (0: config, 1: parameter (system), 2: parameter (user), 3: other hosts.
#endif

CuDlgMain::CuDlgMain(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgMain::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgMain)
	//}}AFX_DATA_INIT
	m_pCurrentPage = NULL;
	m_ingresVariable.Init();

	m_listStrPrecheckIgnore.AddTail(_T("user"));
	m_listStrPrecheckIgnore.AddTail(_T("host name"));
	m_listStrPrecheckIgnore.AddTail(_T("ii_system"));
	m_listStrPrecheckIgnore.AddTail(_T("ii_installation"));

	m_strSnapshot1 = _T("???");
	m_strSnapshot2 = _T("???");
	m_hIconMain = NULL;
	m_pPageOtherHost = NULL;
	m_listMainParam.SetSimpleList(TRUE);
}


void CuDlgMain::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgMain)
	DDX_Control(pDX, IDC_STATIC_MAINICON, m_cMainIcon);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonHostMapping);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgMain, CDialog)
	//{{AFX_MSG_MAP(CuDlgMain)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonHostMapping)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdataData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain message handlers

void CuDlgMain::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgMain::OnDestroy() 
{
	Cleanup();
	if (m_hIconMain)
	DestroyIcon(m_hIconMain);
	CDialog::OnDestroy();
}

void CuDlgMain::Cleanup()
{
	m_listMainParam.DeleteAllItems();
	int i, nCount = m_cTab1.GetItemCount();
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	for (i=0; i<nCount; i++)
	{
		m_cTab1.GetItem(i, &item);
		CWnd* pDlgPage = (CWnd*)item.lParam;
		if (pDlgPage && IsWindow(pDlgPage->m_hWnd))
		{
			pDlgPage->SendMessage(WMUSRMSG_UPDATEDATA, 0, 0);
		}
	}

	while (!m_listDifference.IsEmpty())
		delete m_listDifference.RemoveHead();
	while (!m_listDifferenceOtherHost.IsEmpty())
		delete m_listDifferenceOtherHost.RemoveHead();
	while (!m_lg1.IsEmpty())
		delete m_lg1.RemoveHead();
	while (!m_lg2.IsEmpty())
		delete m_lg2.RemoveHead();

	while (!m_lc1.IsEmpty())
		delete m_lc1.RemoveHead();
	while (!m_lc2.IsEmpty())
		delete m_lc2.RemoveHead();

	while (!m_les1.IsEmpty())
		delete m_les1.RemoveHead();
	while (!m_les2.IsEmpty())
		delete m_les2.RemoveHead();
	while (!m_leu1.IsEmpty())
		delete m_leu1.RemoveHead();
	while (!m_leu2.IsEmpty())
		delete m_leu2.RemoveHead();
	while (!m_lvn1.IsEmpty())
		delete m_lvn1.RemoveHead();
	while (!m_lvn2.IsEmpty())
		delete m_lvn2.RemoveHead();
	while (!m_lOtherhost1.IsEmpty())
		delete m_lOtherhost1.RemoveHead();
	while (!m_lOtherhost2.IsEmpty())
		delete m_lOtherhost2.RemoveHead();
	while (!m_listMappedHost.IsEmpty())
		delete m_listMappedHost.RemoveHead();

	m_strList1Host.RemoveAll();
	m_strList2Host.RemoveAll();
}

void CuDlgMain::HideFrame(int nShow)
{
	ShowWindow (nShow);
}


BOOL CuDlgMain::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_listMainParam.SubclassDlgItem (IDC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK4LISTCTRL, 16, 1, RGB (255, 0, 0));
	m_listMainParam.SetCheckImageList(&m_ImageCheck);
	m_ImageList.Create (IDB_DIFFERENCE, 16, 1, RGB(255, 0, 255));
	m_listMainParam.SetImageList(&m_ImageList, LVSIL_SMALL);

	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TAB_PARAMETER, 220,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_SNAPSHOT1, 150,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_SNAPSHOT2, 150,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_IGNORE,     80,  LVCFMT_CENTER,TRUE}
	};
	InitializeHeader2(&m_listMainParam, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	item.cchTextMax = 32;
#if defined (_VIRTUAL_NODE_AVAILABLE)
	const int nTabcount = 5;
	UINT nIds[nTabcount] =
	{
		IDS_TAB_CONFIG,
		IDS_TAB_ENVSYSTEM,
		IDS_TAB_ENVUSER,
		IDS_TAB_VNODE,
		IDS_TAB_OTHERHOST
	};
#else
	const int nTabcount = 4;
	UINT nIds[nTabcount] =
	{
		IDS_TAB_CONFIG,
		IDS_TAB_ENVSYSTEM,
		IDS_TAB_ENVUSER,
		IDS_TAB_OTHERHOST
	};
#endif

	CString strTabHeader;
	m_cTab1.SetImageList(&m_ImageList);
	int nTabCount = 0;
	CuDlgPageDifference* pDlgPage = NULL;
	for (int i=0; i<nTabcount; i++)
	{
		int nPos = m_cTab1.GetItemCount();
#if defined (_VIRTUAL_NODE_AVAILABLE)
		if (i == 3)
			pDlgPage = new CuDlgPageDifference(TRUE, NULL);
		else
			pDlgPage = new CuDlgPageDifference();
#else
		pDlgPage = new CuDlgPageDifference();
#endif
		pDlgPage->Create(IDD_PAGE_DIFFERENCE, &m_cTab1);
		if (i == OTHER_HOST_INDEX)
		{
			m_pPageOtherHost = pDlgPage;
		}

		strTabHeader.LoadString(nIds[i]);
		item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
		item.lParam  = (LPARAM)pDlgPage; 
		item.iImage  = 3;
		m_cTab1.InsertItem (nPos, &item);
	}
	m_cTab1.DeleteItem(OTHER_HOST_INDEX);
	m_cTab1.SetCurSel(0);
	DisplayPage();

	m_hIconMain = m_ImageList.ExtractIcon(3);
	m_cMainIcon.SetIcon(m_hIconMain);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgMain::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow(m_listMainParam.m_hWnd))
	{
		CRect r, rDlg;
		GetClientRect (rDlg);
		m_listMainParam.GetWindowRect (r);
		ScreenToClient(r);
		r.right = rDlg.right - rDlg.left;
		m_listMainParam.MoveWindow(r);

		m_cTab1.GetWindowRect(r);
		ScreenToClient(r);
		r.right  = rDlg.right  - rDlg.left;
		r.bottom = rDlg.bottom - rDlg.left;
		m_cTab1.MoveWindow(r);
		DisplayPage();
	}
}

void CuDlgMain::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	DisplayPage();
}

void CuDlgMain::DisplayPage()
{
	CRect r;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.cchTextMax = 32;
	int nSel = m_cTab1.GetCurSel();

	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->ShowWindow  (SW_HIDE);
	}
	m_pCurrentPage = NULL;

	if (m_cTab1.GetItem (nSel, &item))
		m_pCurrentPage = (CWnd*)item.lParam;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->MoveWindow (r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}

long CuDlgMain::OnUpdataData (WPARAM wParam, LPARAM lParam)
{
	BOOL bCheck = (BOOL)wParam;
	int  nItem  = (int)lParam;
	CWaitCursor doWaitCursor;
	int i, nCount = m_cTab1.GetItemCount();
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	for (i=0; i<nCount; i++)
	{
		m_cTab1.GetItem(i, &item);
		CWnd* pDlgPage = (CWnd*)item.lParam;
		if (pDlgPage && IsWindow(pDlgPage->m_hWnd))
		{
			pDlgPage->SendMessage(WMUSRMSG_UPDATEDATA, 0, 0);
		}
	}

	//
	// Clean up the current differences:
	POSITION p, pos = m_listDifference.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaCdaDifference* pObj = m_listDifference.GetNext(pos);
		if (pObj->GetType() != CDA_GENERAL)
		{
			m_listDifference.RemoveAt(p);
			delete pObj;
		}
	}
	m_compareParam.CleanIgnore();
	nCount = m_listMainParam.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		CaCdaDifference* pObj = (CaCdaDifference*)m_listMainParam.GetItemData(i);
		CString strName = pObj->GetName();
		strName.MakeLower();
		if (m_listStrPrecheckIgnore.Find(strName) != NULL)
		{
			if (m_listMainParam.GetCheck (i, 3))
			{
				m_compareParam.AddIgnore (strName);
			}
		}
	}
	UINT nMask = PARAM_CONFIGxENV;
	UpdateDifferences(nMask);
	return 0;
}

BOOL CuDlgMain::PrecheckIgnore(LPCTSTR lpszName)
{
	CString strName = lpszName;
	strName.MakeLower();
	return (m_listStrPrecheckIgnore.Find(strName) == NULL)? FALSE: TRUE;
}

void CuDlgMain::UpdateDifferences(UINT nMask)
{
	if (nMask & PARAM_GENERAL)
	{
		VCDA_CompareList(m_lg1, m_lg2, m_listDifference, &m_compareParam);
		DisplayDifference(PARAM_GENERAL);
		nMask &= ~PARAM_GENERAL;
	}

	if (nMask & PARAM_CONFIGxENV)
	{
		VCDA_CompareList(m_lc1,  m_lc2,  m_listDifference, &m_compareParam);
		VCDA_CompareList(m_les1, m_les2, m_listDifference, &m_compareParam);
		VCDA_CompareList(m_leu1, m_leu2, m_listDifference, &m_compareParam);
		VCDA_CompareListVNode(m_lvn1, m_lvn2, m_listDifference, &m_compareParam);
	}

	//
	// Other configured host names:
	if (nMask & PARAM_OTHERHOST)
	{
		CaCompareParam cprm ((const CaCompareParam&)m_compareParam);
		cprm.SetSubstituteHost(1);
		cprm.CleanIgnore();
		cprm.CleanIIDependent();
		cprm.AddIgnore(_T("host name"));
		while (!m_listDifferenceOtherHost.IsEmpty())
			delete m_listDifferenceOtherHost.RemoveHead();

		POSITION pos = m_strList1Host.GetHeadPosition();
		while (pos != NULL)
		{
			CString strH = m_strList1Host.GetNext(pos);
			CaCdaDifference* pDiff = new CaCdaDifference(_T("Unmapped Host"), strH, _T("<not mapped>"), CDA_CONFIG, _T('+'), strH);
			m_listDifferenceOtherHost.AddTail(pDiff);
		}

		pos = m_strList2Host.GetHeadPosition();
		while (pos != NULL)
		{
			CString strH = m_strList2Host.GetNext(pos);
			CaCdaDifference* pDiff = new CaCdaDifference(_T("Unmapped Host"), _T("<not mapped>"), strH, CDA_CONFIG, _T('-'), strH);
			m_listDifferenceOtherHost.AddTail(pDiff);
		}
		VCDA_CompareList2(m_lOtherhost1, m_lOtherhost2, m_listMappedHost, m_listDifferenceOtherHost, &cprm);
	}

	//
	// Display the differences:
	DisplayDifference(nMask);
	UpdateTabImages();
}

static int GetImageIndex(TCHAR c)
{
	int nImage = -1;
	switch (c)
	{
	case _T('+'):
		nImage = 0;
		break;
	case _T('-'):
		nImage = 1;
		break;
	case _T('!'):
		nImage = 2;
		break;
	case _T('='):
		nImage = 3;
		break;
	default:
		break;
	}
	return nImage;
}

void CuDlgMain::DisplayDifference(UINT nMask)
{
	CuDlgPageDifference* pDlgPageOtherHost = NULL;
	CuDlgPageDifference* pDlgPageVNode = NULL;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	m_cTab1.GetItem(0, &item);
	CuDlgPageDifference* pDlgPageConfig = (CuDlgPageDifference*)item.lParam;
	m_cTab1.GetItem(1, &item);
	CuDlgPageDifference* pDlgPageEnvSystem = (CuDlgPageDifference*)item.lParam;
	m_cTab1.GetItem(2, &item);
	CuDlgPageDifference* pDlgPageEnvUser = (CuDlgPageDifference*)item.lParam;
#if defined (_VIRTUAL_NODE_AVAILABLE)
	m_cTab1.GetItem(3, &item);
	pDlgPageVNode = (CuDlgPageDifference*)item.lParam;
#endif
	if (m_cTab1.GetItem(OTHER_HOST_INDEX, &item))
		pDlgPageOtherHost = (CuDlgPageDifference*)item.lParam;

	if (nMask & PARAM_GENERAL)
		m_compareParam.CleanIgnore();
	CString strValue1;
	CString strValue2;
	CaCdaDifference* pObj = NULL;
	POSITION pos = m_listDifference.GetHeadPosition();
	while (pos != NULL)
	{
		int nImage = -1;
		CaCdaDifference* pObj = m_listDifference.GetNext(pos);
		strValue1 = pObj->GetValue1();
		strValue2 = pObj->GetValue2();
		TCHAR& c = pObj->GetDifference();
		switch (c)
		{
		case _T('+'):
			nImage = 0;
			strValue2 = _T("n/a");
			break;
		case _T('-'):
			nImage = 1;
			strValue1 = _T("n/a");
			break;
		case _T('!'):
			nImage = 2;
			break;
		case _T('='):
			nImage = 3;
			break;
		default:
			break;
		}

		if (nMask & PARAM_GENERAL)
		{
			if (c != _T('=') && pObj->GetType() == CDA_GENERAL)
			{
				int nIdx = 0;
				int nCount = m_listMainParam.GetItemCount();
				nIdx = m_listMainParam.InsertItem (nCount, pObj->GetName(), nImage);
				m_listMainParam.SetItemText (nIdx, 1, strValue1);
				m_listMainParam.SetItemText (nIdx, 2, strValue2);
				if (PrecheckIgnore(pObj->GetName()))
				{
					m_listMainParam.SetCheck(nIdx, 3, TRUE);
					m_compareParam.AddIgnore(pObj->GetName());
				}
				m_listMainParam.SetItemData(nCount, (LPARAM)pObj);
			}
		}

		if (nMask & PARAM_CONFIGxENV)
		{
			switch (pObj->GetType())
			{
			case CDA_CONFIG:
				if (pDlgPageConfig)
				{
					if (!pObj->GetHost().IsEmpty())
					{
						if (pObj->GetHost().CompareNoCase(m_compareParam.GetHost1()) != 0 && 
							pObj->GetHost().CompareNoCase(m_compareParam.GetHost2()) != 0 )
							break;
					}
					CListCtrl* pListCtrl = pDlgPageConfig->GetListCtrl();
					int nIdx = 0;
					int nCount = pListCtrl->GetItemCount();
					nIdx = pListCtrl->InsertItem (nCount, pObj->GetName(), nImage);
					pListCtrl->SetItemText (nIdx, 1, strValue1);
					pListCtrl->SetItemText (nIdx, 2, strValue2);
				}
				break;
			case CDA_ENVSYSTEM:
				if (pDlgPageEnvSystem)
				{
					CListCtrl* pListCtrl = pDlgPageEnvSystem->GetListCtrl();
					int nIdx = 0;
					int nCount = pListCtrl->GetItemCount();
					nIdx = pListCtrl->InsertItem (nCount, pObj->GetName(), nImage);
					pListCtrl->SetItemText (nIdx, 1, strValue1);
					pListCtrl->SetItemText (nIdx, 2, strValue2);
				}
				break;
			case CDA_ENVUSER:
				if (pDlgPageEnvUser)
				{
					CListCtrl* pListCtrl = pDlgPageEnvUser->GetListCtrl();
					int nIdx = 0;
					int nCount = pListCtrl->GetItemCount();
					nIdx = pListCtrl->InsertItem (nCount, pObj->GetName(), nImage);
					pListCtrl->SetItemText (nIdx, 1, strValue1);
					pListCtrl->SetItemText (nIdx, 2, strValue2);
				}
				break;
			case CDA_VNODE:
#if defined (_VIRTUAL_NODE_AVAILABLE)
				if (pDlgPageVNode)
				{
					CListCtrl* pListCtrl = pDlgPageVNode->GetListCtrl();
					int nIdx = 0;
					int nCount = pListCtrl->GetItemCount();
					nIdx = pListCtrl->InsertItem (nCount, pObj->GetName(), nImage);
					pListCtrl->SetItemText (nIdx, 1, strValue1);
					pListCtrl->SetItemText (nIdx, 2, strValue2);
				}
#endif
				break;
			default:
				break;
			}
		}
	}

	//
	// Other configured host names:
	if (nMask && PARAM_OTHERHOST)
	{
		pos = m_listDifferenceOtherHost.GetHeadPosition();
		while (pos != NULL)
		{
			CaCdaDifference* pObj = m_listDifferenceOtherHost.GetNext(pos);
			strValue1 = pObj->GetValue1();
			strValue2 = pObj->GetValue2();
			TCHAR& c = pObj->GetDifference();
			int nImage = GetImageIndex(c);
			switch (c)
			{
			case _T('+'):
				strValue2 = _T("n/a");
				break;
			case _T('-'):
				strValue1 = _T("n/a");
				break;
			default:
				break;
			}

			switch (pObj->GetType())
			{
			case CDA_CONFIG:
				if (pDlgPageOtherHost)
				{
					CListCtrl* pListCtrl = pDlgPageOtherHost->GetListCtrl();
					int nIdx = 0;
					int nCount = pListCtrl->GetItemCount();
					nIdx = pListCtrl->InsertItem (nCount, pObj->GetName(), nImage);
					pListCtrl->SetItemText (nIdx, 1, strValue1);
					pListCtrl->SetItemText (nIdx, 2, strValue2);
				}
				break;
			default:
				break;
			}
		}
	}
}


void CuDlgMain::UpdateTabImages()
{
	int i, nCount = m_cTab1.GetItemCount();
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	for (i=0; i<nCount; i++)
	{
		m_cTab1.GetItem(i, &item);

		CuDlgPageDifference* pPage = (CuDlgPageDifference*)item.lParam;
		if (pPage)
		{
			TCITEM its;
			memset (&its, 0, sizeof (its));
			its.mask = TCIF_IMAGE;
			CListCtrl* pListCtrl = pPage->GetListCtrl();
			its.iImage = (pListCtrl->GetItemCount() == 0)? 3: 2;
			m_cTab1.SetItem(i, &its);
		}
	}

	if (m_hIconMain)
		DestroyIcon(m_hIconMain);
	if (m_listMainParam.GetItemCount() == 0)
		m_hIconMain = m_ImageList.ExtractIcon(3);
	else
		m_hIconMain = m_ImageList.ExtractIcon(2);
	m_cMainIcon.SetIcon(m_hIconMain);
}

void CuDlgMain::CheckHostName (int nConfig, CaCda* pObj, CaCompareParam* pCompareInfo)
{
	CStringList* pList = (nConfig == 1)? &m_strList1Host: &m_strList2Host;
	CString strSnapshotHost = (nConfig == 1)? pCompareInfo->GetHost1(): pCompareInfo->GetHost2();
	CString strLeft = pObj->GetLeft();
	strLeft.MakeLower();
	if (strLeft.Find(_T("ii.*.")) != 0)
	{
		//
		// Parse host name:
		CString str = pObj->GetLeft();
		CString strHost = _T("");
		int nFound = str.Find(_T('.'));
		if (nFound != -1)
		{
			str = str.Mid(nFound +1);
			nFound = str.Find(_T('.'));
			if (nFound != -1)
				strHost = str.Left(nFound);
		}

		if (!strHost.IsEmpty() && strHost.CompareNoCase(strSnapshotHost) != 0 && pList->Find(strHost) == NULL)
			pList->AddTail(strHost);
	}
}


void CuDlgMain::PrepareData(CTypedPtrList< CObList, CaCda* >& ls1, CTypedPtrList< CObList, CaCda* >& ls2)
{
	Cleanup();

	int i;
	POSITION pos;
	CTypedPtrList< CObList, CaCda* >* ls [2] = {&ls1,    &ls2};
	CTypedPtrList< CObList, CaCda* >* lg [2] = {&m_lg1,  &m_lg2};
	CTypedPtrList< CObList, CaCda* >* lc [2] = {&m_lc1,  &m_lc2};
	CTypedPtrList< CObList, CaCda* >* les[2] = {&m_les1, &m_les2};
	CTypedPtrList< CObList, CaCda* >* leu[2] = {&m_leu1, &m_leu2};
	CTypedPtrList< CObList, CaCda* >* lvn[2] = {&m_lvn1, &m_lvn2};

	for (i=0; i<2; i++)
	{
		while (!ls[i]->IsEmpty())
		{
			CaCda* pObj = ls[i]->RemoveHead();
			switch (pObj->GetType())
			{
			case CDA_GENERAL:
				if (pObj->GetName().CompareNoCase(_T("PLATFORM")) == 0)
				{
					if (i==0)
						m_compareParam.SetPlatform1 (pObj->GetValue());
					else
						m_compareParam.SetPlatform2 (pObj->GetValue());
				}
				lg[i]->AddTail(pObj);
				m_compareParam.ManageGeneralParameters(i+1, pObj);
				break;
			case CDA_CONFIG:
				//
				// Check if the left string "NAME" in "NAME: VALUE" contains the host name.
				// If so, construct the list of unique host names. If the list contains more than one 
				// elelent, then there are multiple hosts configured in a config.dat file.
				CheckHostName (i+1, pObj, &m_compareParam);
				lc[i]->AddTail(pObj);
				break;
			case CDA_ENVSYSTEM:
				les[i]->AddTail(pObj);
				break;
			case CDA_ENVUSER:
				leu[i]->AddTail(pObj);
				break;
			case CDA_VNODE:
				lvn[i]->AddTail(pObj);
				break;
			default:
				ASSERT(FALSE);
				delete pObj;
				break;
			}
		}
	}

	//
	// Manage the other configured host names:
	CStringArray arrayHost;
	arrayHost.Add(m_compareParam.GetHost1());
	arrayHost.Add(m_compareParam.GetHost2());
	CTypedPtrList< CObList, CaCda* >* lhost[2] = {&m_lOtherhost1, &m_lOtherhost2};
	for (i=0; i<2; i++)
	{
		POSITION p = NULL;
		pos = lc[i]->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaCda* pObj = lc[i]->GetNext (pos);
			if (!pObj->GetHost().IsEmpty() && arrayHost[i].CompareNoCase (pObj->GetHost()) != 0)
			{
				lc[i]->RemoveAt(p);
				lhost[i]->AddTail(pObj);
			}
		}
	}
	
	//
	// Add a line: Other Hosts Configured into a main parameters:
	CString strListH1 = _T("");
	CString strListH2 = _T("");
	BOOL bOne = TRUE;
	pos = m_strList1Host.GetHeadPosition();
	while (pos != NULL)
	{
		CString strH = m_strList1Host.GetNext(pos);
		if (!bOne)
			strListH1 += _T(", ");
		strListH1 += strH;
		bOne = FALSE;
	}
	bOne = TRUE;
	pos = m_strList2Host.GetHeadPosition();
	while (pos != NULL)
	{
		CString strH = m_strList2Host.GetNext(pos);
		if (!bOne)
			strListH2 += _T(", ");
		strListH2 += strH;
		bOne = FALSE;
	}
	m_lg1.AddTail(new CaCda(_T("Other Hosts Configured"), strListH1, CDA_GENERAL));
	m_lg2.AddTail(new CaCda(_T("Other Hosts Configured"), strListH2, CDA_GENERAL));

	//
	// Default other hosts mapping:
	while (!m_strList1Host.IsEmpty() && !m_strList2Host.IsEmpty())
	{
		CString strHost1 = m_strList1Host.RemoveHead();
		CString strHost2 = m_strList2Host.RemoveHead();
		m_listMappedHost.AddTail(new CaMappedHost(strHost1, strHost2));
	}

	if (!((m_strList1Host.IsEmpty() || m_strList2Host.IsEmpty()) && m_listMappedHost.IsEmpty()))
		m_cButtonHostMapping.ShowWindow(SW_SHOW);
	else
		m_cButtonHostMapping.ShowWindow(SW_HIDE);


	//
	// Make the Tab "Other Hosts Configured" appear or not:
#if defined (_VIRTUAL_NODE_AVAILABLE)
	int nMaxTab = 5;
#else
	int nMaxTab = 4;
#endif

	CString strTabHeader;
	int nTab = m_cTab1.GetItemCount();
	if (m_strList1Host.IsEmpty() && m_strList2Host.IsEmpty())
	{
		m_pPageOtherHost->ShowWindow(SW_HIDE);
		m_cTab1.DeleteItem(OTHER_HOST_INDEX);
		nTab = m_cTab1.GetItemCount();
	}
	else
	{
		CString strTabHeader;
		if (nTab < nMaxTab)
		{
			TCITEM item;
			memset (&item, 0, sizeof (item));
			item.mask       = TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
			item.cchTextMax = 32;
			strTabHeader.LoadString(IDS_TAB_OTHERHOST);
			item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
			item.lParam  = (LPARAM)m_pPageOtherHost; 
			item.iImage  = 3;
			m_cTab1.InsertItem (nTab, &item);
		}
	}
	if (m_cTab1.GetCurSel() == -1)
	{
		m_cTab1.SetCurSel(0);
		DisplayPage();
	}

	//
	// Change Tab label depending the network!
	if (VCDA_IsNetworkInstallation())
	{
		TCITEM item;
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_TEXT;
		item.cchTextMax = 32;
#if defined (_VIRTUAL_NODE_AVAILABLE)
		const int nTabcount = 5;
		UINT nIds[nTabcount] =
		{
			IDS_TAB_CONFIGxLOCAL,
			IDS_TAB_ENVSYSTEMxLOCAL,
			IDS_TAB_ENVUSER,
			IDS_TAB_VNODE,
			IDS_TAB_OTHERHOST
		};
#else
		const int nTabcount = 4;
		UINT nIds[nTabcount] =
		{
			IDS_TAB_CONFIGxLOCAL,
			IDS_TAB_ENVSYSTEMxLOCAL,
			IDS_TAB_ENVUSER,
			IDS_TAB_OTHERHOST
		};
#endif

		int nTab = m_cTab1.GetItemCount();
		m_cTab1.SetImageList(&m_ImageList);
		for (int i=0; i<nTab; i++)
		{
			strTabHeader.LoadString(nIds[i]);
			item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
			m_cTab1.SetItem(i, &item);
		}
	}


}


void CuDlgMain::OnButtonHostMapping() 
{
	CxDlgHostMapping dlg;
	dlg.SetParams(m_strList1Host, m_strList2Host, m_listMappedHost);
	dlg.SetSnapshotCaptions(m_strSnapshot1, m_strSnapshot2);

	int nAnswer = dlg.DoModal();
	if (nAnswer == IDOK)
	{
		CWaitCursor doWaitCursor;
		TCITEM item;
		memset (&item, 0, sizeof (item));
		item.mask = TCIF_PARAM;
		if (m_cTab1.GetItem(OTHER_HOST_INDEX, &item)) // Other host 
		{
			CWnd* pDlgPage = (CWnd*)item.lParam;
			if (pDlgPage)
			{
				pDlgPage->SendMessage(WMUSRMSG_UPDATEDATA, 0, 0);
			}
		}
		UINT nMask = PARAM_OTHERHOST;
		UpdateDifferences(nMask);
	}
}

void CuDlgMain::SetSnapshotCaptions(LPCTSTR lpSnapshot1, LPCTSTR lpSnapshot2)
{
	CuDlgPageDifference* pDlgPageOtherHost = NULL;
	CuDlgPageDifference* pDlgPageVNode = NULL;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	m_cTab1.GetItem(0, &item);
	CuDlgPageDifference* pDlgPageConfig = (CuDlgPageDifference*)item.lParam;
	m_cTab1.GetItem(1, &item);
	CuDlgPageDifference* pDlgPageEnvSystem = (CuDlgPageDifference*)item.lParam;
	m_cTab1.GetItem(2, &item);
	CuDlgPageDifference* pDlgPageEnvUser = (CuDlgPageDifference*)item.lParam;
#if defined (_VIRTUAL_NODE_AVAILABLE)
	m_cTab1.GetItem(3, &item);
	pDlgPageVNode = (CuDlgPageDifference*)item.lParam;
#endif
	if (m_cTab1.GetItem(OTHER_HOST_INDEX, &item))
		pDlgPageOtherHost = (CuDlgPageDifference*)item.lParam;

#if defined (_VIRTUAL_NODE_AVAILABLE)
	const int lsCount = 6;
	CListCtrl* pLsctrl [lsCount] = 
	{
		&m_listMainParam, 
		&(pDlgPageConfig->m_cListCtrl),
		&(pDlgPageEnvSystem->m_cListCtrl),
		&(pDlgPageEnvUser->m_cListCtrl),
		&(pDlgPageVNode->m_cListCtrl),
		&(m_pPageOtherHost->m_cListCtrl)
	};
#else
	const int lsCount = 5;
	CListCtrl* pLsctrl [lsCount] = 
	{
		&m_listMainParam, 
		&(pDlgPageConfig->m_cListCtrl),
		&(pDlgPageEnvSystem->m_cListCtrl),
		&(pDlgPageEnvUser->m_cListCtrl),
		&(m_pPageOtherHost->m_cListCtrl)
	};
#endif

	LVCOLUMN vc;
	memset (&vc, 0, sizeof (vc));
	vc.mask = LVCF_TEXT;
	for (int i=0; i<lsCount; i++)
	{
		vc.pszText = (LPTSTR)(LPCTSTR)lpSnapshot1;
		pLsctrl[i]->SetColumn(1, &vc);
		vc.pszText = (LPTSTR)(LPCTSTR)lpSnapshot2;
		pLsctrl[i]->SetColumn(2, &vc);
	}

	m_strSnapshot1 = lpSnapshot1;
	m_strSnapshot2 = lpSnapshot2;
}


void CuDlgMain::OnRestore()
{
	// TODO: The property sheet attached to your project
	// via this function is not hooked up to any message
	// handler.  In order to actually use the property sheet,
	// you will need to associate this function with a control
	// in your project such as a menu item or tool bar button.
	POSITION pos = m_listDifference.GetHeadPosition();
	while (pos != NULL)
	{
		CaCdaDifference* pDiff = m_listDifference.GetNext(pos);
		if (pDiff->GetType() != CDA_GENERAL)
			break;
		if (pDiff->GetName().CompareNoCase(_T("VERSION")) == 0)
		{
			CString strMsg = _T("");
			CString strV1,strV2;
			int iPos1,iPos2;
			int iePos1,iePos2;
			iPos1 = pDiff->GetValue1().Find(_T('('));
			iPos2 = pDiff->GetValue2().Find(_T('('));
			iePos1 = pDiff->GetValue1().Find(_T('/'));
			iePos2 = pDiff->GetValue2().Find(_T('/'));
			if (iPos1!=-1 && (iePos1 > iPos1))
				strV1 = pDiff->GetValue1().Mid(iPos1,(iePos1-iPos1));
			else
				strV1 = pDiff->GetValue1();

			if (iPos2!=-1 && (iePos2 > iPos2))
				strV2 = pDiff->GetValue2().Mid(iPos2,(iePos2-iPos2));
			else
				strV2 = pDiff->GetValue2();

			strV1.TrimLeft();
			strV1.TrimRight();
			strV2.TrimLeft();
			strV2.TrimRight();
			if (strV1.CompareNoCase(strV2) != 0)
			{
				AfxFormatString2(strMsg, IDS_MSG_DIFF_PLATFORM, (LPCTSTR)strV1, (LPCTSTR)strV2);
				AfxMessageBox (strMsg);
				return;
			}
		}
	}

	CxPSheetRestore propSheet(m_listDifference, NULL);
	propSheet.m_Page3.SetSnapshot(m_strSnapshot2);

	CaRestoreParam& data = propSheet.GetData();
	data.m_plg1 = &m_lg1;
	data.m_plg2 = &m_lg2;
	data.m_plc1 = &m_lc1;
	data.m_plc2 = &m_lc2;
	data.m_ples1= &m_les1;
	data.m_ples2= &m_les2;
	data.m_pleu1= &m_leu1;
	data.m_pleu2= &m_leu2;
	data.m_plvn1= &m_lvn1;
	data.m_plvn2= &m_lvn2;
	data.m_plOtherhost1= &m_lOtherhost1;
	data.m_plOtherhost2= &m_lOtherhost2;
	data.m_plistDifference = &m_listDifference;
	data.m_pIngresVariable = &m_ingresVariable;

	int nAnswer = propSheet.DoModal();

	// This is where you would retrieve information from the property
	// sheet if propSheet.DoModal() returned IDOK.  We aren't doing
	// anything for simplicity.
	if (nAnswer != IDCANCEL)
	{
		CuVcdaCtrl* pParent = (CuVcdaCtrl*)GetParent();
		if (!pParent)
			return;
		pParent->DoCompare();
	}
}

BOOL CuDlgMain::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo(m_hWnd, 40001);
	return CDialog::OnHelpInfo(pHelpInfo);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vcda.chm");
	}

	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}
