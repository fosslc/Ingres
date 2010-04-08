/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/
/*
**
**    Source   : ckplst.cpp, implementation file
**
**
**    Project  : Ingres Journal Analyser
**    Author   : Schalk Philippe
**
**    Purpose  : Manage dialog box for display list of checkpoints.
**
** History:
**
** 02-Dec-2003 (schph01)
**    SIR #111389 add in project for displayed the checkpoint list.
**/
#include "stdafx.h"
#include "remotecm.h"
#include "ckplst.h"
#include "ijactrl.h"
#include "rctools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// interface to auditdb.c and rollfwd.c
extern "C" int MfcDlgCheckPointLst(char *szDBname,char *szOwnerName,
                                   char *szVnodeName, char *szCurrChkPtNum)
{
	int ires;
	CString csCurChkPt = szCurrChkPtNum;
	CxDlgCheckPointLst Dlg;
	Dlg.m_csCurDBName    = szDBname;
	Dlg.m_csCurDBOwner   = szOwnerName;
	Dlg.m_csCurVnodeName = szVnodeName;
	Dlg.SetSelectedCheckPoint(csCurChkPt);
	ires = Dlg.DoModal();

	if (ires != IDCANCEL)
		strcpy(szCurrChkPtNum , (LPTSTR)(LPCTSTR)Dlg.GetSelectedCheckPoint());

	return ires;
}



/////////////////////////////////////////////////////////////////////////////
// CxDlgCheckPointLst dialog


CxDlgCheckPointLst::CxDlgCheckPointLst(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgCheckPointLst::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgCheckPointLst)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgCheckPointLst::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgCheckPointLst)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgCheckPointLst, CDialog)
	//{{AFX_MSG_MAP(CxDlgCheckPointLst)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgCheckPointLst message handlers

BOOL CxDlgCheckPointLst::OnInitDialog() 
{
	CString csTitle,csNewTitle;
	CDialog::OnInitDialog();

	GetWindowText(csTitle);
	csNewTitle.Format(csTitle, m_csCurVnodeName ,m_csCurDBName);
	SetWindowText(csNewTitle);


	InitializeListCtrl();
	fillListCtrl();

	if ( !m_cListCtrl.GetItemCount() )
		GetDlgItem(IDOK)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgCheckPointLst::InitializeListCtrl()
{
	//
	// subclass columns list control and connect image list
	//
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST_CHECKPOINT, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);

	m_ImageCheck.Create (IDB_CHECK4LISTCTRL_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);
	m_ImageList.Create(1, 18, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	#define LAYOUT_NUMBER  (6)
	LSCTRLHEADERPARAMS2  aColumns[LAYOUT_NUMBER] =
	{
		{ IDS_TC_DATE,         190,LVCFMT_LEFT  ,FALSE},
		{ IDS_TC_CKPSEQUENCE,  110,LVCFMT_LEFT  ,FALSE},
		{ IDS_TC_FIRSTJNL,     60 ,LVCFMT_LEFT  ,FALSE},
		{ IDS_TC_LASTJNL,      60 ,LVCFMT_LEFT  ,FALSE},
		{ IDS_TC_VALID,        50 ,LVCFMT_CENTER,TRUE },
		{ IDS_TC_MODE,         200,LVCFMT_LEFT  ,FALSE}
	}; 
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, aColumns, LAYOUT_NUMBER);

	m_cListCtrl.SetForegroundWindow();
}

void CxDlgCheckPointLst::fillListCtrl()
{
	if (m_csCurDBName.IsEmpty() || m_csCurDBOwner.IsEmpty() || m_csCurVnodeName.IsEmpty())
		return;

	m_CaCheckPointList.m_csDB_Name    = m_csCurDBName;
	m_CaCheckPointList.m_csDB_Owner   = m_csCurDBOwner;
	m_CaCheckPointList.m_csVnode_Name = m_csCurVnodeName;
	m_CaCheckPointList.RetrieveCheckPointList();

	CaCheckPoint* pCheckPt = NULL;
	POSITION pos = m_CaCheckPointList.GetCheckPointList().GetHeadPosition();
	int index, nCount=m_cListCtrl.GetItemCount();
	while (pos != NULL)
	{
		pCheckPt = m_CaCheckPointList.GetCheckPointList().GetNext (pos);
		if (pCheckPt)
		{
			index = m_cListCtrl.InsertItem(nCount,pCheckPt->GetDate());
			m_cListCtrl.SetItemText (index, 1, pCheckPt->GetCheckSequence());
			m_cListCtrl.SetItemText (index, 2, pCheckPt->GetFirstJnl());
			m_cListCtrl.SetItemText (index, 3, pCheckPt->GetLastJnl());
			if (_tcsncmp(pCheckPt->GetValid(),_T("1"),1) == 0)
				m_cListCtrl.SetCheck(index, 4,TRUE);
			else
				m_cListCtrl.SetCheck(index, 4,FALSE);
			m_cListCtrl.SetItemText (index, 5, pCheckPt->GetMode());

			if ( m_csCurrentSelectedCheckPoint == pCheckPt->GetCheckSequence())
			{
				m_cListCtrl.SetItemState(index,LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
				m_cListCtrl.EnsureVisible( index, FALSE );
			}

			nCount++;
		}
	}
}

void CxDlgCheckPointLst::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CxDlgCheckPointLst::OnOK() 
{
	int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);

	if (selItemId != -1)
		m_csCurrentSelectedCheckPoint = m_cListCtrl.GetItemText(selItemId,1);
	else
		m_csCurrentSelectedCheckPoint = _T("");

	CDialog::OnOK();
}

BOOL CxDlgCheckPointLst::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo (m_hWnd, IDD_CHECKPOINT_LIST);
	return TRUE;
}
