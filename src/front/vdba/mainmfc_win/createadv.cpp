/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  createadv.cpp : Creates a panel for Advanced tab in CreateDB
**                  dialog.
**
** History:
**	08-Feb-2010 (drivi01)
**		Created.
*/


#include "stdafx.h"
#include "createadv.h"
extern "C"
{
#include "cv.h"
}

// CuDlgcreateDBAdvanced
LPCREATEDB m_StructDatabaseInfo;
CxDlgCreateAlterDatabase* pParentDlg;
void CheckAllProducts (CuCheckListBox* pListBox);


static CTLDATA prodTypes [] =
{
	PROD_INGRES,        IDS_PROD_INGRES,
	PROD_VISION,        IDS_PROD_VISION,
	PROD_W4GL,          IDS_PROD_W4GL,
	// PROD_NOFECLIENTS,   IDS_PROD_NOFECLIENTS,
	-1// Terminator
};
static CTLDATA prodTypesNoW4GL [] =
{
	PROD_INGRES,        IDS_PROD_INGRES,
	PROD_VISION,        IDS_PROD_VISION,
	// PROD_NOFECLIENTS,   IDS_PROD_NOFECLIENTS,
	-1// Terminator
};

CuDlgcreateDBAdvanced::CuDlgcreateDBAdvanced(CWnd* pParent)
	: CDialogEx(CuDlgcreateDBAdvanced::IDD, pParent)
{

}

CuDlgcreateDBAdvanced::~CuDlgcreateDBAdvanced()
{
}


void CuDlgcreateDBAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgcreateDBAdvanced)
	DDX_Control(pDX, IDC_READ_ONLY, m_cbReadOnly);
	DDX_Control(pDX, IDC_STATIC_CDB, m_ctrlStaticCoordName);
	DDX_Control(pDX, IDC_PUBLIC, m_ctrlPublic);
	DDX_Control(pDX, IDC_GENFE, m_ctrlGenerateFrontEnd);
	DDX_Control(pDX, IDC_DISTRIBUTED, m_ctrlDistributed);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LANGUAGE, m_ctrlLanguage);
	DDX_Control(pDX, IDC_COORDNAME, m_ctrlCoordName);
	DDX_Control(pDX, IDC_COMBO_CAT_PAGE_SIZE, m_CtrlCatalogsPageSize);
	DDX_Control(pDX, IDC_STATIC_CAT_PAGE_SIZE, m_CtrlStaticCatPageSize);
	DDX_Control(pDX, IDC_FRONT_END_LIST, m_CheckFrontEndList);
	//}}AFX_DATA_MAP
}




BEGIN_MESSAGE_MAP(CuDlgcreateDBAdvanced, CDialogEx)
	//{{AFX_MSG_MAP(CxDlgCreateAlterDatabase)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_DISTRIBUTED, OnDistributed)
	ON_BN_CLICKED(IDC_GENFE, OnGenerateFrontEnd)
	ON_BN_CLICKED(IDC_READ_ONLY, OnReadOnly)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CuDlgcreateDBAdvanced::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	VERIFY (m_CheckFrontEndList.SubclassDlgItem (IDC_FRONT_END_LIST, this));

	pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
	m_StructDatabaseInfo = pParentDlg->m_StructDatabaseInfo;


	// fill checklistbox with products
	if (!OccupyProductsControl())
	{
		ASSERT(NULL);
		EndDialog (-1);
		return TRUE;
	}

	// check PUBLIC or PRIVATE
	CheckRadioButton( IDC_PUBLIC, IDC_PRIVATE, m_StructDatabaseInfo->bPrivate ? IDC_PRIVATE : IDC_PUBLIC);

	// initialise DBname ,Language ,Coordinator
	InitialiseEditControls();

	//SelectDefaultLocation();
	if ( GetOIVers() >= OIVERS_30)
	{
		m_CtrlCatalogsPageSize.SetCurSel(0);
		if (m_StructDatabaseInfo->bAlter)
		{
			long lVal;
			CVal((LPSTR)m_StructDatabaseInfo->szCatalogsPageSize,&lVal);
			InitPageSize (lVal);
		}
		else
			InitPageSize (0L);
	}
	else
	{
		m_CtrlStaticCatPageSize.ShowWindow(SW_HIDE);
		m_CtrlCatalogsPageSize.ShowWindow(SW_HIDE);
	}

	// hide Star and FrontEnd related controls
	if (m_StructDatabaseInfo->bAlter)
	{
		m_cbReadOnly.EnableWindow(FALSE);
		m_ctrlLanguage.EnableWindow(FALSE);
		if (m_StructDatabaseInfo->bDistributed)
		{
			m_ctrlDistributed.SetCheck(1);
			m_ctrlDistributed.EnableWindow(FALSE);
			m_ctrlStaticCoordName.ShowWindow(SW_SHOW);
			m_ctrlStaticCoordName.EnableWindow(FALSE);
			m_ctrlCoordName.ShowWindow(SW_SHOW);
			m_ctrlCoordName.EnableWindow(FALSE);
		}
		else
		{
			m_ctrlDistributed.EnableWindow(FALSE);
			m_ctrlStaticCoordName.EnableWindow(FALSE);
			m_ctrlCoordName.EnableWindow(FALSE);
		}

		if (m_StructDatabaseInfo->bGenerateFrontEnd)
		{
			m_CheckFrontEndList.ShowWindow(SW_SHOW);
			m_CheckFrontEndList.EnableWindow(TRUE);
			m_ctrlGenerateFrontEnd.SetCheck(TRUE);
			m_ctrlGenerateFrontEnd.EnableWindow(FALSE);
			int i,nCount;
			BOOL bCheck=FALSE;
			nCount = m_CheckFrontEndList.GetCount();
			for (i=0; i<nCount; i++)
			{
				DWORD dwtype = m_CheckFrontEndList.GetItemData(i);
				switch (dwtype)
				{
					case PROD_INGRES :
						if (m_StructDatabaseInfo->szCatOIToolsAvailables[TOOLS_INGRES]=='Y')
							bCheck = TRUE;
						break;
					case PROD_VISION :
						if (m_StructDatabaseInfo->szCatOIToolsAvailables[TOOLS_VISION]=='Y')
							bCheck = TRUE;
						break;
					case PROD_W4GL :
						if (m_StructDatabaseInfo->szCatOIToolsAvailables[TOOLS_W4GL]=='Y')
							bCheck = TRUE;
						break;
				}
				if (bCheck) {
					m_CheckFrontEndList.SetCheck(i);
					m_CheckFrontEndList.EnableItem(i,FALSE);
				}
				bCheck = FALSE;
			}
		}
		else
			m_CheckFrontEndList.ShowWindow(SW_HIDE);
	}
	else
	{
		m_ctrlStaticCoordName.ShowWindow(SW_SHOW);
		m_ctrlCoordName.ShowWindow(SW_SHOW);
		m_ctrlStaticCoordName.EnableWindow(FALSE);
		m_ctrlCoordName.EnableWindow(FALSE);
		// the default now is with the "Generate Front End catalogs" checked.
		m_ctrlGenerateFrontEnd.SetCheck(TRUE);
		OnGenerateFrontEnd(); // simulate ON_BN_CLICKED on checkbox "Generate Front End catalogs"
	}

	return TRUE;

}

BOOL CuDlgcreateDBAdvanced::OccupyProductsControl()
{
	int i=0;
	LPCTLDATA lpdata;

	if (pParentDlg->IsW4GLInLocalInstall())
		lpdata = prodTypes;
	else
		lpdata = prodTypesNoW4GL;

	m_CheckFrontEndList.ResetContent();
	while (lpdata[i].nId != -1)
	{
		CString Tempo;
		int nIdx;
		Tempo.LoadString(lpdata[i].nStringId);
		nIdx = m_CheckFrontEndList.AddString(Tempo);
		if (nIdx != LB_ERR)
			m_CheckFrontEndList.SetItemData(nIdx, lpdata[i].nId);
		i++;
	}
	if (m_CheckFrontEndList.GetCount())
		return TRUE;
	else
		return FALSE;
}

void CuDlgcreateDBAdvanced::OnReadOnly() 
{
	CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
	
	if ( m_cbReadOnly.GetCheck() == BST_CHECKED )
	{
		m_ctrlDistributed.SetCheck(BST_UNCHECKED);
		m_ctrlDistributed.EnableWindow(FALSE);
		OnDistributed();
		m_ctrlGenerateFrontEnd.SetCheck(BST_UNCHECKED);
		m_ctrlGenerateFrontEnd.EnableWindow(FALSE);
		OnGenerateFrontEnd();
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocDatabase.EnableWindow(TRUE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocWork.EnableWindow(FALSE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocWork.SetCurSel(0);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocJournal.EnableWindow(FALSE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocJournal.SetCurSel(0);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.EnableWindow(FALSE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.SetCurSel(0);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocDump.EnableWindow(FALSE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocDump.SetCurSel(0);
		CheckRadioButton(IDC_PUBLIC, IDC_PRIVATE, IDC_PUBLIC);
		m_ctrlPublic.EnableWindow(FALSE);
		GetDlgItem(IDC_PRIVATE)->EnableWindow(FALSE);
		m_ctrlLanguage.SetWindowText("");
		m_ctrlLanguage.EnableWindow(FALSE);
	}
	else
	{
		m_ctrlDistributed.EnableWindow(TRUE);
		OnDistributed();
		m_ctrlGenerateFrontEnd.EnableWindow(TRUE);
		OnGenerateFrontEnd();
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocDatabase.EnableWindow(TRUE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocWork.EnableWindow(TRUE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocJournal.EnableWindow(TRUE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.EnableWindow(TRUE);
		pParentDlg->m_pDlgcreateDBLocation->m_ctrlLocDump.EnableWindow(TRUE);
		m_ctrlPublic.EnableWindow(TRUE);
		GetDlgItem(IDC_PRIVATE)->EnableWindow(TRUE);
		m_ctrlLanguage.EnableWindow(TRUE);
	}
	
}

void CuDlgcreateDBAdvanced::InitialiseEditControls()
{
	m_ctrlLanguage.SetLimitText( MAXOBJECTNAME-1);
	m_ctrlCoordName.SetLimitText( MAXOBJECTNAME-1);

	m_ctrlLanguage.SetWindowText((LPCTSTR)m_StructDatabaseInfo->Language);
	m_ctrlCoordName.SetWindowText(_T(""));
}

void CuDlgcreateDBAdvanced::OnDistributed() 
{
	if (m_ctrlDistributed.GetCheck() == BST_CHECKED)
	{
		m_ctrlStaticCoordName.ShowWindow(SW_SHOW);
		m_ctrlCoordName.ShowWindow(SW_SHOW);
		m_ctrlStaticCoordName.EnableWindow(TRUE);
		m_ctrlCoordName.EnableWindow(TRUE);
		UpdateCoordName();
		CheckRadioButton(IDC_PUBLIC, IDC_PRIVATE, IDC_PUBLIC); // force to public
		CButton*  cbPrivate = (CButton*)GetDlgItem (IDC_PRIVATE);
		cbPrivate->EnableWindow(FALSE);
	}
	else
	{
		m_ctrlStaticCoordName.ShowWindow(SW_SHOW);
		m_ctrlCoordName.ShowWindow(SW_SHOW);
		m_ctrlStaticCoordName.EnableWindow(FALSE);
		m_ctrlCoordName.EnableWindow(FALSE);
		CButton*  cbPrivate = (CButton*)GetDlgItem (IDC_PRIVATE);
		cbPrivate->EnableWindow(TRUE);
	}
}

void CuDlgcreateDBAdvanced::OnGenerateFrontEnd() 
{

	if (m_ctrlGenerateFrontEnd.GetCheck() == BST_CHECKED)
	{
		m_CheckFrontEndList.ShowWindow(SW_SHOW);
		CheckAllProducts(&m_CheckFrontEndList);
	}
	else
		m_CheckFrontEndList.ShowWindow(SW_HIDE);
}

void CheckAllProducts (CuCheckListBox* pListBox)
{
	if (!pListBox)
		return;
	if (!IsWindow (pListBox->m_hWnd))
		return;
	int i, nCount;
	nCount = pListBox->GetCount();
	for (i=0; i<nCount; i++)
	{
		pListBox->SetCheck(i);
	}
}

void CuDlgcreateDBAdvanced::UpdateCoordName()
{
	CString csBufDBname,csBuffSet;
	pParentDlg->m_ctrlDatabaseName.GetWindowText( csBufDBname );
	if (!csBufDBname.IsEmpty())
		csBuffSet = _T("ii") + csBufDBname;
	else
		csBuffSet.Empty();
	m_ctrlCoordName.SetWindowText( csBuffSet );
}

void CuDlgcreateDBAdvanced::InitPageSize (LONG lPage_size)
{
	int i, index,nMax;
	typedef struct tagCxU
	{
		char* cx;
		LONG  ux;
	} CxU;
	CxU pageSize [] =
	{
		{"Default", 0L},
		{" 2K", 2048L},
		{" 4K", 4096L},
		{" 8K", 8192L},
		{"16K",16384L},
		{"32K",32768L},
		{"64K",65536L},
		{'\0', 0}
	};

	for (i=0; pageSize[i].cx != 0; i++)
	{
		if ( m_StructDatabaseInfo->bAlter && pageSize [i].ux == 0L)
			continue; // skip Default in Alter Mode
		index = m_CtrlCatalogsPageSize.AddString (pageSize [i].cx);
		if (index != -1)
			m_CtrlCatalogsPageSize.SetItemData (index, (DWORD)pageSize [i].ux);
	}
	m_CtrlCatalogsPageSize.SetCurSel(0);
	nMax = m_CtrlCatalogsPageSize.GetCount();
	for (i=0; i<nMax; i++)
	{
		if ( m_CtrlCatalogsPageSize.GetItemData(i) == lPage_size)
		{
			m_CtrlCatalogsPageSize.SetCurSel(i);
			break;
		}
	}
}

void CuDlgcreateDBAdvanced::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CuDlgcreateDBAdvanced::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}


// CuDlgcreateDBAdvanced message handlers


