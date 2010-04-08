/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : chooseob.cpp : implementation file
**    Project  : VDBA / Generic window control 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modal dialog that displays items in the list control and
**               allows user to select the objects 
**               (The list control behaves like CCheckListBox)
**
** History:
**
** 27-Apr-2001 (uk$so01)
**    Created for SIR #102678
**    Support the composite histogram of base table and secondary index.
**/



#include "stdafx.h"
#include "rcdepend.h"
#include "chooseob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CxChooseObjects::CxChooseObjects(CWnd* pParent /*=NULL*/)
	: CDialog(CxChooseObjects::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxChooseObjects)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pArrayHeader = NULL;
	m_nHeaderSize = 0;
	m_pfnDisplay = NULL;
	m_pSmallImageList = NULL;
	m_strTitle = _T("Choose Objects");
	m_nIDHelp = 0;
}


void CxChooseObjects::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxChooseObjects)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxChooseObjects, CDialog)
	//{{AFX_MSG_MAP(CxChooseObjects)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CxChooseObjects::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowText (m_strTitle);

	m_StateImageList.Create(IDB_CHECK, 16, 1, RGB(255, 0, 0));
	m_cList1.SetLineSeperator(FALSE);

	VERIFY (m_cList1.SubclassDlgItem (IDC_LIST1, this));
	m_cList1.SetImageList(&m_StateImageList, LVSIL_STATE);
	if (m_pSmallImageList)
		m_cList1.SetImageList(m_pSmallImageList, LVSIL_SMALL);

	//
	// You must initialize the headers first. See the member: InitializeHeader()
	int i, nItemCount;
	LVCOLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	ASSERT(m_pArrayHeader);
	if (m_pArrayHeader && m_nHeaderSize > 0)
	{
		for (i=0; i<m_nHeaderSize; i++)
		{
			lvcolumn.fmt      = m_pArrayHeader[i].m_fmt;
			lvcolumn.pszText  = (LPTSTR)m_pArrayHeader[i].m_tchszHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx       = m_pArrayHeader[i].m_cxWidth;
			m_cList1.InsertColumn (i, &lvcolumn);
		}
	}

	nItemCount = m_arrayObjects.GetSize();
	ASSERT (nItemCount > 0);
	if (nItemCount > 0)
	{
		LVITEM lvi;
		for(i = 0; i < nItemCount; i++)
		{
			lvi.mask = LVIF_PARAM | LVIF_STATE;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			lvi.lParam = (LPARAM)m_arrayObjects.GetAt(i);
			lvi.stateMask = LVIS_STATEIMAGEMASK;
			lvi.state = INDEXTOSTATEIMAGEMASK(1);
			lvi.iImage = i;

			m_cList1.InsertItem(&lvi);
		}
	}
	Display();
#if defined (_USE_PUSHHELPID)
	PushHelpId (m_nIDHelp);
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void* CxChooseObjects::GetSelectObject(void* pObject)
{
	int i, nSize = m_arraySelectedObjects.GetSize();
	for (i=0; i<nSize; i++)
	{
		if (pObject == (void*)m_arraySelectedObjects.GetAt(i))
			return pObject;
	}
	return NULL;
}

void CxChooseObjects::Display()
{
	ASSERT(m_pfnDisplay);
	if (!m_pfnDisplay)
		return;

	int i, nItemCount = m_cList1.GetItemCount();
	for (i=0; i<nItemCount; i++)
	{
		void* pItemData = (void*)m_cList1.GetItemData(i);
		m_pfnDisplay(pItemData, &m_cList1, i);
		if (GetSelectObject(pItemData))
		{
			m_cList1.CheckItem (i, TRUE);
		}
	}
}


void CxChooseObjects::OnOK() 
{
	m_arraySelectedObjects.RemoveAll();
	int i, nCount = m_cList1.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		BOOL bChecked = m_cList1.GetItemChecked(i);
		if (bChecked)
		{
			void* pObject = (void*)m_cList1.GetItemData(i);
			m_arraySelectedObjects.Add (pObject);
		}
	}
	
	CDialog::OnOK();
}

void CxChooseObjects::OnDestroy() 
{
	CDialog::OnDestroy();
#if defined (_USE_PUSHHELPID)
	PopHelpId();
#endif
}