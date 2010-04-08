/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xtbpkey.cpp, Header File
**    Project  : Ingres II./ VDBA
**    Author   : UK Sotheavut
**    Purpose  : Popup Dialog Box for Creating a Primary Key of Table
**
** History:
**
** xx-10-1998 (uk$so01)
**    Created
** 24-Feb-2000 (uk$so01)
**    BUG #100560. Disable the Parallel index creation and index enhancement
**    if not running against the DBMS 2.5 or later.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "xtbpkey.h"
#include "xdgidxop.h"

extern "C"
{
#include "dbadlg1.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CxDlgTablePrimaryKey::CxDlgTablePrimaryKey(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgTablePrimaryKey::IDD, pParent)
{
	if (!m_cButtonUp.LoadBitmaps  (BTN_UP_UP,   BTN_UP_DOWN,   BTN_UP_FOCUS,   BTN_UP_GREY)   ||
		!m_cButtonDown.LoadBitmaps(BTN_DOWN_UP, BTN_DOWN_DOWN, BTN_DOWN_FOCUS, BTN_DOWN_GREY))
	{
		TRACE0("Failed to load bitmaps for buttons\n");
		AfxThrowResourceException();
	}
	m_strDatabase  = _T("");
	m_strTable     = _T("");
	m_strTableOwner= _T("");

	//{{AFX_DATA_INIT(CxDlgTablePrimaryKey)
	m_strKeyName = _T("");
	//}}AFX_DATA_INIT
	m_pKeyParam = NULL;
	m_bAlter = FALSE;
	m_nDelMode = 0;
	m_pTable    = NULL;
}


void CxDlgTablePrimaryKey::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgTablePrimaryKey)
	DDX_Control(pDX, IDC_MFC_BUTTON5, m_cButtonDeleteColumn);
	DDX_Control(pDX, IDC_MFC_BUTTON4, m_cButtonAddColumn);
	DDX_Control(pDX, IDC_MFC_EDITKEYNAME, m_cEditKeyName);
	DDX_Control(pDX, IDC_MFC_BUTTON3, m_cButtonIndex);
	DDX_Control(pDX, IDC_MFC_BUTTON2, m_cButtonDeleteCascade);
	DDX_Control(pDX, IDC_MFC_BUTTON1, m_cButtonDelete);
	DDX_Control(pDX, IDC_MFC_LIST2, m_cListKey);
	DDX_Text(pDX, IDC_MFC_EDITKEYNAME, m_strKeyName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgTablePrimaryKey, CDialog)
	//{{AFX_MSG_MAP(CxDlgTablePrimaryKey)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnButtonDelete)
	ON_BN_CLICKED(IDC_MFC_BUTTON2, OnButtonDeleteCascade)
	ON_BN_CLICKED(IDC_MFC_BUTTON3, OnButtonIndex)
	ON_BN_CLICKED(IDC_MFC_BUTTON4, OnAddColumn)
	ON_BN_CLICKED(IDC_MFC_BUTTON5, OnButtonDropColumn)
	ON_BN_CLICKED(IDC_MFC_BUTTON6, OnButtonUpColumn)
	ON_BN_CLICKED(IDC_MFC_BUTTON7, OnButtonDownColumn)
	ON_NOTIFY(HDN_ITEMCHANGED, IDC_MFC_LIST1, OnItemchangedMfcList1)
	ON_NOTIFY(NM_DBLCLK, IDC_MFC_LIST1, OnDblclkMfcList1)
	ON_LBN_DBLCLK(IDC_MFC_LIST2, OnDblclkMfcList2)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgTablePrimaryKey message handlers

void CxDlgTablePrimaryKey::OnButtonDelete() 
{
	m_cListKey.ResetContent();
	m_cEditKeyName.SetWindowText (_T(""));
	m_nDelMode = 1;
	EnableControls();
}

void CxDlgTablePrimaryKey::OnButtonDeleteCascade() 
{
	m_cListKey.ResetContent();
	m_cEditKeyName.SetWindowText (_T(""));
	m_nDelMode = 2;
	EnableControls();
}

void CxDlgTablePrimaryKey::OnButtonIndex() 
{
	ASSERT (m_pKeyParam);
	if (!m_pKeyParam)
		return;
	LPTABLEPARAMS pTable = (LPTABLEPARAMS)m_pTable;

	CxDlgIndexOption dlg;
	dlg.m_strTitle.LoadString(IDS_T_PRIMARY_KEY);// = _T("Primary Key Index Enforcement");
	dlg.m_nCallFor = 0;
	dlg.m_pParam = (LPVOID)&(m_pKeyParam->constraintWithClause);
	dlg.m_strDatabase = m_strDatabase;
	dlg.SetTableParams ((LPVOID)m_pTable);
	
	if (m_bAlter)
		dlg.SetAlter();
	if (!pTable->bCreate)
	{
		dlg.m_strTable = (LPCTSTR)StringWithoutOwner (pTable->objectname);
		dlg.m_strTableOwner = (LPCTSTR)StringWithoutOwner (pTable->szSchema);
		dlg.m_bFromCreateTable = FALSE;
	}
	else
		dlg.m_bFromCreateTable = TRUE;

	int answer = dlg.DoModal();
	if (answer == IDOK)
	{
		INDEXOPTION_IDX2STRUCT (&dlg, (LPVOID)&(m_pKeyParam->constraintWithClause));
	}
}

void CxDlgTablePrimaryKey::OnAddColumn() 
{
	if (m_bAlter)
		return;

	CString strItem;
	int iIndex = -1;
	int i, idx, nCount = m_cListCtrl.GetItemCount();

	for (i=0; i<nCount; i++)
	{
		if (m_cListCtrl.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			strItem = m_cListCtrl.GetItemText (i, 0);
			if (m_cListKey.FindStringExact (-1, strItem) != LB_ERR)
				continue;
			if (m_cListCtrl.GetCheck(i, 1) == TRUE)
			{
				//CString strMsg = _T("Nullable column cannot be part of primary key.");
				AfxMessageBox (VDBA_MfcResourceString(IDS_E_NULLABLE_COL));
				continue;
			}
			idx = m_cListKey.AddString(strItem);
		}
	}
	EnableControls();
}

void CxDlgTablePrimaryKey::OnButtonDropColumn() 
{
	if (m_bAlter)
		return;

	int nCurSel = m_cListKey.GetCurSel();
	if (nCurSel == LB_ERR)
		return;
	m_cListKey.DeleteString (nCurSel);
	int nCount = m_cListKey.GetCount();
	if (nCount > 0)
	{
		if (nCurSel == nCount)
			m_cListKey.SetCurSel(nCurSel -1);
		else
			m_cListKey.SetCurSel(nCurSel);
	}
	EnableControls();
}

void CxDlgTablePrimaryKey::OnButtonUpColumn() 
{
	int nCount  = m_cListKey.GetCount();
	int nCurSel = m_cListKey.GetCurSel();
	if (nCount < 2)
		return;
	if (nCurSel < 1)
		return;
	CString strItem;
	m_cListKey.GetText (nCurSel, strItem);
	m_cListKey.DeleteString (nCurSel);
	m_cListKey.InsertString (nCurSel-1, strItem);
	m_cListKey.SetCurSel (nCurSel-1);
}

void CxDlgTablePrimaryKey::OnButtonDownColumn() 
{
	int nCount  = m_cListKey.GetCount();
	int nCurSel = m_cListKey.GetCurSel();
	if (nCount < 2)
		return;
	if (nCurSel == (nCount -1))
		return;
	CString strItem;
	m_cListKey.GetText (nCurSel, strItem);
	m_cListKey.DeleteString (nCurSel);
	m_cListKey.InsertString (nCurSel+1, strItem);
	m_cListKey.SetCurSel (nCurSel+1);
}

void CxDlgTablePrimaryKey::OnOK() 
{
	CDialog::OnOK();
	ASSERT (m_pKeyParam);
	if (!m_pKeyParam)
		return;
	if (!m_bAlter)
	{
		m_strKeyName.TrimLeft();
		m_strKeyName.TrimRight();
		lstrcpy (m_pKeyParam->tchszPKeyName, m_strKeyName);
	}
	//
	// Get the list of column of primary key:
	if (!m_bAlter)
	{
		m_pKeyParam->pListKey = StringList_Done (m_pKeyParam->pListKey);
		CString strItem;
		int i, nCount = m_cListKey.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cListKey.GetText (i, strItem);
			m_pKeyParam->pListKey = StringList_Add (m_pKeyParam->pListKey, strItem);
		}
	}
	if (m_bAlter)
		m_pKeyParam->nMode = m_nDelMode;
}

void CxDlgTablePrimaryKey::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

BOOL CxDlgTablePrimaryKey::OnInitDialog() 
{
	ASSERT (m_pKeyParam);
	if (!m_pKeyParam)
		return FALSE;
	CDialog::OnInitDialog();
	VERIFY(m_cButtonUp.SubclassDlgItem(IDC_MFC_BUTTON6, this));
	VERIFY(m_cButtonDown.SubclassDlgItem(IDC_MFC_BUTTON7, this));
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);

	LV_COLUMN lvcolumn;
	TCHAR     szColHeader [2][16];
	lstrcpy(szColHeader [0],VDBA_MfcResourceString(IDS_TC_COLUMN));   // _T("Column")
	lstrcpy(szColHeader [1],VDBA_MfcResourceString(IDS_TC_NULLABLE)); // _T("Nullable")
	int       i;
	int       hWidth [2] = {130, 80};
	//
	// Set the number of columns: 2
	// List of columns of Base Table:
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<2; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = szColHeader[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i==0)
			m_cListCtrl.InsertColumn (i, &lvcolumn); 
		else
		{
			lvcolumn.fmt = LVCFMT_CENTER;
			m_cListCtrl.InsertColumn (i, &lvcolumn, TRUE);
		}
	}
	m_cEditKeyName.LimitText (33);
	LPSTRINGLIST ls = m_pKeyParam->pListTableColumn;
	while (ls != NULL)
	{
		AddTableColumn (ls->lpszString, (ls->extraInfo1==0)? TRUE: FALSE);
		ls = ls->next;
	}

	ls = m_pKeyParam->pListKey;
	while (ls != NULL)
	{
		m_cListKey.AddString (ls->lpszString);
		ls = ls->next;
	}
	m_nDelMode = m_pKeyParam->nMode;
	m_bAlter = m_pKeyParam->bAlter;
	SetDisplayMode();
	m_cButtonDeleteCascade.ShowWindow (m_bAlter? SW_SHOW: SW_HIDE);
	EnableControls();

	//
	// Hide the button index enhancement if not Ingres >= 2.5
	if ( GetOIVers() < OIVERS_25 ) 
	{
		m_cButtonIndex.ShowWindow (SW_HIDE);
	}

	PushHelpId(m_bAlter? HELPID_IDD_PRIMARYKEY_ALTER: HELPID_IDD_PRIMARYKEY_ADD);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgTablePrimaryKey::AddTableColumn (LPCTSTR lpszColumn, BOOL bNullable)
{
	int nCount = m_cListCtrl.GetItemCount();

	m_cListCtrl.InsertItem  (nCount, lpszColumn);
	m_cListCtrl.SetCheck    (nCount, 1, bNullable);
	m_cListCtrl.EnableCheck (nCount, 1, FALSE);
}

void CxDlgTablePrimaryKey::OnItemchangedMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CxDlgTablePrimaryKey::SetDisplayMode()
{
	m_cButtonDelete.EnableWindow (TRUE);
	if (!m_bAlter)
		return;
	m_cButtonDeleteCascade.EnableWindow (TRUE);

	m_cButtonUp.EnableWindow (FALSE);
	m_cButtonDown.EnableWindow (FALSE);

	m_cButtonAddColumn.EnableWindow (FALSE);
	m_cButtonDeleteColumn.EnableWindow (FALSE);
	
	m_cEditKeyName.SetReadOnly (TRUE);
}

void CxDlgTablePrimaryKey::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnAddColumn();
	*pResult = 0;
}

void CxDlgTablePrimaryKey::OnDblclkMfcList2() 
{
	OnButtonDropColumn();
}

void CxDlgTablePrimaryKey::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgTablePrimaryKey::EnableControls()
{
	int nCount = m_cListKey.GetCount();

	if (nCount == 0)
	{
		m_cButtonDelete.EnableWindow (FALSE);
		m_cButtonDeleteCascade.EnableWindow (FALSE);
		m_cButtonIndex.EnableWindow (FALSE);
	}
	else
	{
		m_cButtonDelete.EnableWindow (TRUE);
		m_cButtonDeleteCascade.EnableWindow (TRUE);
		m_cButtonIndex.EnableWindow (TRUE);
	}
}
