/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Source   : ModSize.cpp, implementation file
**
**  Project  : Ingres/Visual DBA.
**
**  Author   : Philippe Schalk 
**
**  Purpose  : Dialog for selected available page size before a 'Alter Table'
**
**  29-Jun-2001 (schph01)
**     (SIR 105158) created
**  29-Jun-2004 (schph01)
**     (BUG 112571) Change the variable type, pass by function
*      SQLGetTableStorageStructure() because this variable is fill by lstrcpy
**     and cannot be a CString.
*/

#include "stdafx.h"
#include "mainmfc.h"

extern "C" {
#include "dbaset.h"
#include "dbaginfo.h"
#include "msghandl.h"
  extern LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer);
}
#include "modsize.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// interface to altertbl.c
extern "C" int MfcModifyPageSize( LPTABLEPARAMS lpTS)
{
	CxDlgModifyPageSize dlg;
	dlg.m_lpTblParams = lpTS;

	return dlg.DoModal();
}



/////////////////////////////////////////////////////////////////////////////
// CxDlgModifyPageSize dialog


CxDlgModifyPageSize::CxDlgModifyPageSize(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgModifyPageSize::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgModifyPageSize)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgModifyPageSize::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgModifyPageSize)
	DDX_Control(pDX, IDOK, m_ctrlOKButton);
	DDX_Control(pDX, IDC_STATIC_CBF_CONFIG, m_ChangeDBMS_PageSize);
	DDX_Control(pDX, IDC_LIST_PAGE_SIZE, m_ctrlLstPageSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgModifyPageSize, CDialog)
	//{{AFX_MSG_MAP(CxDlgModifyPageSize)
	ON_LBN_SELCHANGE(IDC_LIST_PAGE_SIZE, OnSelchangeListPageSize)
	ON_LBN_KILLFOCUS(IDC_LIST_PAGE_SIZE, OnKillfocusListPageSize)
	ON_LBN_SETFOCUS(IDC_LIST_PAGE_SIZE, OnSetfocusListPageSize)
	ON_LBN_SELCANCEL(IDC_LIST_PAGE_SIZE, OnSelcancelListPageSize)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgModifyPageSize message handlers

void CxDlgModifyPageSize::OnOK()
{
	CString csConnect,csStatement;
	TCHAR tcStructureStorage[17];
	int ires,isess,iIndex;
	long iPageSize;

	iIndex = m_ctrlLstPageSize.GetCurSel();
	if (iIndex != LB_ERR)
		iPageSize = (long)m_ctrlLstPageSize.GetItemData(iIndex);
	else
	{
		AfxMessageBox(IDS_E_NO_SELECTED_PAGES);//"no page size has been selected."
		return;
	}
	csConnect.Format("%s::%s", m_lpTblParams->szNodeName,m_lpTblParams->DBName);
	ires = Getsession((LPUCHAR)(LPCTSTR)csConnect,SESSION_TYPE_CACHENOREADLOCK, &isess);
	if (ires != RES_SUCCESS)
	{
		AfxMessageBox(IDS_E_GET_SESSION);
		return;
	}
	// Get the current storage structure.
	ires = SQLGetTableStorageStructure(m_lpTblParams, tcStructureStorage);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(isess, RELEASE_ROLLBACK);
		ErrorMessage(IDS_E_NO_STORAGE_STRUCTURE,RES_ERR);
		return;
	}
	csStatement.Format("modify %s.%s to %s with page_size = %d", QuoteIfNeeded((LPTSTR)m_lpTblParams->szSchema),
	                                                             QuoteIfNeeded((LPTSTR)m_lpTblParams->objectname),
	                                                             tcStructureStorage,
	                                                             iPageSize);

	ires=ExecSQLImm((LPUCHAR)(LPCTSTR)csStatement,FALSE, NULL, NULL, NULL);
	if (ires == RES_ERR)
	{
		ReleaseSession(isess, RELEASE_ROLLBACK);
		ErrorMessage(IDS_E_MODIFY_PAGE_SIZE_FAILED,RES_ERR);//"Modify Page Size Failed."
		return;
	}

	ReleaseSession(isess, RELEASE_COMMIT);
	// update the current page size in structure.
	m_lpTblParams->uPage_size = iPageSize;
	CDialog::OnOK();
}

BOOL CxDlgModifyPageSize::OnInitDialog()
{
	CDialog::OnInitDialog();

	FillPageSize();
	EnableDisableOKButton();
	PushHelpId (IDD_MODIFY_PAGE_SIZE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CxDlgModifyPageSize::FillPageSize()
{
	TCHAR tcEnabled[4];
	CString csConnect;
	int ires,isess,iIndex,i;
	typedef struct tagCxPage
	{
		char* cName;
		LONG  uValue;
	} CxPage;
	CxPage pageSize [] =
	{
		{"page_size_2K", 2048L },
		{"page_size_4K", 4096L },
		{"page_size_8K", 8192L },
		{"page_size_16K",16384L},
		{"page_size_32K",32768L},
		{"page_size_64K",65536L},
		{NULL           , -1   }
	};

	if (GetOIVers() >= OIVERS_20)
	{
		csConnect.Format("%s::%s", m_lpTblParams->szNodeName,m_lpTblParams->DBName);
		ires = Getsession((LPUCHAR)(LPCTSTR)csConnect,SESSION_TYPE_CACHENOREADLOCK, &isess);
		if (ires == RES_SUCCESS)
		{
			for (i=0; pageSize[i].cName != NULL; i++)
			{
				INGRESII_llDBMSInfo(pageSize[i].cName,tcEnabled);
				if ((tcEnabled[0] == _T('Y') || tcEnabled[0] == _T('y')) &&
					m_lpTblParams->uPage_size < pageSize[i].uValue)
				{
					iIndex = m_ctrlLstPageSize.AddString(pageSize[i].cName);
					m_ctrlLstPageSize.SetItemData(iIndex,(DWORD)pageSize[i].uValue);
				}
			}
		}
		ReleaseSession(isess, RELEASE_COMMIT);
	}
	return TRUE;
}

void CxDlgModifyPageSize::OnSelchangeListPageSize() 
{
	EnableDisableOKButton();
}

void CxDlgModifyPageSize::OnKillfocusListPageSize() 
{
	EnableDisableOKButton();
}

void CxDlgModifyPageSize::OnSetfocusListPageSize() 
{
	EnableDisableOKButton();
}

void CxDlgModifyPageSize::OnSelcancelListPageSize() 
{
	EnableDisableOKButton();
}

void CxDlgModifyPageSize::EnableDisableOKButton()
{
	int nSel = m_ctrlLstPageSize.GetCurSel();
	if (nSel == LB_ERR) // no items selected grayed OK button
		m_ctrlOKButton.EnableWindow(FALSE);
	else
		m_ctrlOKButton.EnableWindow(TRUE);

}

void CxDlgModifyPageSize::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
