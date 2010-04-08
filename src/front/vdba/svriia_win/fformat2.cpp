/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat2.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (Fixed Width format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "resource.h"
#include "fformat2.h"
#include "formdata.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgPageFixedWidth, CDialog)
CuDlgPageFixedWidth::CuDlgPageFixedWidth(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgPageFixedWidth::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgPageFixedWidth)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pArrayPos = NULL;
	m_bReadOnly = FALSE;
}

CuDlgPageFixedWidth::CuDlgPageFixedWidth(CWnd* pParent, BOOL bReadOnly)
	: CDialog(CuDlgPageFixedWidth::IDD, pParent)
{
	m_pArrayPos = NULL;
	m_bReadOnly = bReadOnly;
	m_wndFixedWidth.SetReadOnly (m_bReadOnly);
}


void CuDlgPageFixedWidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPageFixedWidth)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckConfirmChoice);
	DDX_Control(pDX, IDC_STATIC_FRAME, m_cStatic1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPageFixedWidth, CDialog)
	//{{AFX_MSG_MAP(CuDlgPageFixedWidth)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckConfirmChoice)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CLEANDATA,  OnCleanData)
	ON_MESSAGE (WMUSRMSG_GETITEMDATA,OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPageFixedWidth message handlers
void CuDlgPageFixedWidth::OnDestroy() 
{
	if (m_pArrayPos)
		delete m_pArrayPos;
	m_pArrayPos = NULL;
	CDialog::OnDestroy();
}

void CuDlgPageFixedWidth::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgPageFixedWidth::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CRect r;


	m_cStatic1.GetWindowRect (r);
	ScreenToClient (r);

	m_wndFixedWidth.SetReadOnly(m_bReadOnly);
	m_wndFixedWidth.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE/*|WS_CLIPSIBLINGS|WS_CLIPCHILDREN*/, r, this, 2500);
	m_wndFixedWidth.MoveWindow (r);
	m_wndFixedWidth.ShowWindow (SW_SHOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPageFixedWidth::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_wndFixedWidth.m_hWnd))
	{
		CRect r, r2, rClient;
		GetClientRect (rClient);
		m_cStatic1.GetWindowRect (r);
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
		m_wndFixedWidth.MoveWindow (r);
	}
	
}

LONG CuDlgPageFixedWidth::OnCleanData (WPARAM wParam, LPARAM lParam)
{
	//
	// Do not call m_wndFixedWidth.Cleanup(); because we want to preserve
	// the dividers that user has marked:
	CuListBoxFixedWidth& lb = m_wndFixedWidth.GetListBox();
	lb.ResetContent();
	m_wndFixedWidth.ResizeControls();

	return 0;
}

LONG CuDlgPageFixedWidth::OnGetData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	switch (nMode)
	{
	case 0: // Separator Type
		return (LONG)FMT_FIXEDWIDTH;
	case 1: // Array of divider positions
		{
			int* pArrayPosition = NULL;
			int  nSize = 0;
			m_wndFixedWidth.GetColumnPositions(pArrayPosition, nSize);
			return (LONG)pArrayPosition;
		}
	case 3: // Column Count
		return (long)m_wndFixedWidth.GetColumnDividerCount();
	case CHECK_CONFIRM_CHOICE:
		return m_cCheckConfirmChoice.GetCheck();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	return 0;
}

//
// wParam = CHECK_CONFIRM_CHOICE (Update the control m_cCheckConfirmChoice. lParam = 0|1)
// wParam = 0 (General, lParam = address of class CaIIAInfo)
LONG CuDlgPageFixedWidth::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	if (nMode == CHECK_CONFIRM_CHOICE)
	{
		int nCheck = (int)lParam;
		m_cCheckConfirmChoice.SetCheck (nCheck);
		return 0;
	}

	CaIIAInfo* pData = (CaIIAInfo*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0;
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	
	int nSize = 0;
	int nField= 0;
	int nIdx  = 0;
	
	CStringArray& arrayString = dataPage1.GetArrayTextLine();
	nSize = arrayString.GetSize();
	m_wndFixedWidth.SetData (arrayString, dataPage1.GetLineCountToSkip());
	m_wndFixedWidth.Display (0);

	int i, nArraySize = 0;
	if (m_pArrayPos)
	{
		int nArraySize = lstrlen (m_pArrayPos);
		for (i=0; i<nArraySize; i++)
		{
			if (m_pArrayPos[i] == _T(' '))
			m_wndFixedWidth.AddDivider (CPoint ((i+1), 0));
		}
	}

	return TRUE;
}



void CuDlgPageFixedWidth::OnCheckConfirmChoice() 
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
