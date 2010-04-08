/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ckplst.cpp, implementation file
**
**
**    Project  : Ingres II / Visual DBA.
**    Author   : Schalk Philippe
**
**    Purpose  : Manage dialog box for display list of checkpoints.
**
** History:
**
** 02-Dec-2003 (schph01)
**    SIR #111389 adjust the width for 'date' and 'mode' columns.
*******************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "ckplst.h"

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

	PushHelpId(IDD_CHECKPOINT_LIST);

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
	m_ImageList.Create(1, 18, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);

	#define LAYOUT_NUMBER  (6)
	CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] =
	{
		{ _T(""), FALSE, 200},
		{ _T(""), FALSE, -1 },
		{ _T(""), FALSE, -1 },
		{ _T(""), FALSE, -1 },
		{ _T(""), TRUE,  -1 },
		{ _T(""), FALSE, 210}
	};

	lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_DATE       ));
	lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_CKPSEQUENCE));
	lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_FIRSTJNL   ));
	lstrcpy(aColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_LASTJNL    ));
	lstrcpy(aColumns[4].szCaption, VDBA_MfcResourceString(IDS_TC_VALID      ));
	lstrcpy(aColumns[5].szCaption, VDBA_MfcResourceString(IDS_TC_MODE       ));
	InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);
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
			if (x_strncmp(pCheckPt->GetValid(),_T("1"),1) == 0)
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

	PopHelpId();
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
