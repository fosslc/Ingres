/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat1.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (DBF format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "resource.h"
#include "fformat1.h"
#include "iapsheet.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgPageDbf::CuDlgPageDbf(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgPageDbf::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgPageDbf)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pData = NULL;
	memset (&m_sizeText, 0, sizeof(m_sizeText));
	m_sizeText.cx =  9;
	m_sizeText.cx = 16;
	m_nCoumnCount = -1;
	m_bFirstUpdate = TRUE;
}


void CuDlgPageDbf::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPageDbf)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckConfirmChoice);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPageDbf, CDialog)
	//{{AFX_MSG_MAP(CuDlgPageDbf)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckConfirmChoice)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CLEANDATA,  OnCleanData)
	ON_MESSAGE (WMUSRMSG_GETITEMDATA,OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPageDbf message handlers

void CuDlgPageDbf::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgPageDbf::OnInitDialog() 
{
	CDialog::OnInitDialog();
	try
	{
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;

		CRect r;
		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		m_listCtrlHuge.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, r, this, IDC_LIST1);
		m_listCtrlHuge.MoveWindow (r);
		m_listCtrlHuge.ShowWindow (SW_SHOW);
		m_listCtrlHuge.SetFullRowSelected(TRUE);
		m_listCtrlHuge.SetLineSeperator(TRUE, TRUE, FALSE);
		m_listCtrlHuge.SetAllowSelect(FALSE);
		m_listCtrlHuge.SetDisplayFunction(DisplayListCtrlHugeItem, NULL);
	}
	catch (...)
	{
		TRACE0("Raise exception at CuDlgPageDbf::OnInitDialog()\n");
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPageDbf::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_listCtrlHuge.m_hWnd))
	{
		CRect r, r2, rClient;
		GetClientRect (rClient);
		m_listCtrlHuge.GetWindowRect (r);
		ScreenToClient (r);
		int nCxMargin = r.left;
		r.bottom = rClient.bottom - nCxMargin;

#if defined (CONFIRM_TABCHOICE_WITH_CHECKBOX)
		//
		// Checkbox confirm choice:
		m_cCheckConfirmChoice.GetWindowRect (r2);
		ScreenToClient (r2);
		int nH = r2.Height();
		r2.bottom = rClient.bottom - nCxMargin;
		r2.top = r2.bottom - nH;
		m_cCheckConfirmChoice.MoveWindow(r2);
		m_cCheckConfirmChoice.EnableWindow(TRUE);
		m_cCheckConfirmChoice.ShowWindow(SW_SHOW);

		r.bottom = r2.top-4;
#endif

		r.right = rClient.right - nCxMargin;
		m_listCtrlHuge.MoveWindow (r);
		m_listCtrlHuge.ResizeToFit();
	}
}

//
// wParam = CHECK_CONFIRM_CHOICE (Update the control m_cCheckConfirmChoice. lParam = 0|1)
// wParam = 0 (General, lParam = address of class CaIIAInfo)
LONG CuDlgPageDbf::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	if (nMode == CHECK_CONFIRM_CHOICE)
	{
		int nCheck = (int)lParam;
		m_cCheckConfirmChoice.SetCheck (nCheck);
		return 0;
	}

	m_pData = (CaIIAInfo*)lParam;
	ASSERT (m_pData);
	if (!m_pData)
		return 0;
	CaDataPage1& dataPage1 = m_pData->m_dataPage1;
	CaDataPage2& dataPage2 = m_pData->m_dataPage2;
	m_nCoumnCount = dataPage2.GetColumnCount();

	CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
	int i, nSize = arrayRecord.GetSize();
	m_bFirstUpdate = FALSE;
	CStringArray& arrayColumns = dataPage2.GetArrayColumns();
	ASSERT (m_nCoumnCount == arrayColumns.GetSize());
	//
	// Initialize headers:
	LSCTRLHEADERPARAMS* pLsp = new LSCTRLHEADERPARAMS[m_nCoumnCount];
	for (i=0; i<m_nCoumnCount && i<arrayColumns.GetSize(); i++)
	{
		CString strHeader = arrayColumns.GetAt(i);

#if defined (MAXLENGTH_AND_EFFECTIVE)
		CString strSize;
		int& nFieldMax = dataPage2.GetFieldSizeMax(i);
		int& nFieldEff = dataPage2.GetFieldSizeEffectiveMax(i);
		strSize.Format(_T(" (%d)"), nFieldMax);
		strHeader+=strSize;
#endif

		pLsp[i].m_fmt = LVCFMT_LEFT;
		if (strHeader.GetLength() > MAXLSCTRL_HEADERLENGTH)
			lstrcpyn (pLsp[i].m_tchszHeader, strHeader, MAXLSCTRL_HEADERLENGTH);
		else
			lstrcpy  (pLsp[i].m_tchszHeader, strHeader);
		pLsp[i].m_cxWidth= 100;
		pLsp[i].m_bUseCheckMark = FALSE;
	}

	m_listCtrlHuge.InitializeHeader(pLsp, m_nCoumnCount);
	m_listCtrlHuge.SetDisplayFunction(DisplayListCtrlHugeItem, NULL);
	m_listCtrlHuge.SetSharedData (&arrayRecord);
	delete pLsp;

	//
	// Display the new records:
	m_listCtrlHuge.ResizeToFit();

#if defined (CONFIRM_TABCHOICE_WITH_CHECKBOX)
	m_cCheckConfirmChoice.SetCheck(1);
#endif

	return TRUE;
}

LONG CuDlgPageDbf::OnCleanData (WPARAM wParam, LPARAM lParam)
{
	if (!m_pData)
		return 0;
	//
	// No not destroy the records. 
	// Because in the DBF case, the records are not queried in
	// the member OnUpdateData(), instead they are queried in STEP1 OnWizardNext()
	// It depends on the STEP1 to clean the data
	return 0;
}

LONG CuDlgPageDbf::OnGetData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	switch (nMode)
	{
	case 0: // Separator Type
		return (LONG)FMT_DBF;
	case 3: // Column Count
		return (long)m_nCoumnCount;
	case CHECK_CONFIRM_CHOICE:
		return m_cCheckConfirmChoice.GetCheck();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	return 0;
}


void CuDlgPageDbf::OnCheckConfirmChoice() 
{
	//
	// You have already checked one.\nThe previous check will be cancelled.
	CString strMsg;
	strMsg.LoadString(IDS_CONFIRM_CHOICE_WITH_CHECKBOX);

	CWnd* pParent = GetParent();
	ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CTabCtrl)));
	if (pParent && pParent->IsKindOf(RUNTIME_CLASS(CTabCtrl)))
	{
		BOOL nAsked = FALSE;
		TCITEM item;
		memset (&item, 0, sizeof (item));
		item.mask = TCIF_PARAM;
		CTabCtrl* pTab = (CTabCtrl*)pParent;
		int nCheck, i, nCount = pTab->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (pTab->GetItem (i, &item))
			{
				CWnd* pPage = (CWnd*)item.lParam;
				ASSERT (pPage);
				if (!pPage)
					continue;
				if (pPage == this)
					continue;
				nCheck = pPage->SendMessage(WMUSRMSG_GETITEMDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
				if (nCheck == 1)
				{
					if (!nAsked)
					{
						nAsked = TRUE;
						int nAnswer = AfxMessageBox (strMsg, MB_OKCANCEL|MB_ICONEXCLAMATION);
						if (nAnswer != IDOK)
						{
							m_cCheckConfirmChoice.SetCheck (0);
							return;
						}
						//
						// Uncheck the checkbox m_cCheckConfirmChoice:
						pPage->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
					}
					else
					{
						//
						// Uncheck the checkbox m_cCheckConfirmChoice:
						pPage->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
					}
				}
			}
		}
	}
}
