// localter.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "localter.h"

#include "createdb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBAlternateLoc dialog


CuDlgcreateDBAlternateLoc::CuDlgcreateDBAlternateLoc(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgcreateDBAlternateLoc::IDD, pParent)
{
	//m_lpDBExtensions=NULL;
	//{{AFX_DATA_INIT(CuDlgcreateDBAlternateLoc)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgcreateDBAlternateLoc::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgcreateDBAlternateLoc)
	DDX_Control(pDX, IDC_STATIC_WK, m_StaticWK);
	DDX_Control(pDX, IDC_STATIC_DB, m_StaticDB);
	DDX_Control(pDX, IDC_STATIC_TEXT, m_StaticText);
	DDX_Control(pDX, IDC_WORK_ALTERNATE, m_cbWorkAlternate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgcreateDBAlternateLoc, CDialog)
	//{{AFX_MSG_MAP(CuDlgcreateDBAlternateLoc)
	ON_BN_CLICKED(IDC_WORK_ALTERNATE, OnWorkAlternate)
	//}}AFX_MSG_MAP
    ON_CLBN_CHKCHANGE (IDC_ALTERNATE_LOC_WORK, OnCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBAlternateLoc message handlers

void CuDlgcreateDBAlternateLoc::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgcreateDBAlternateLoc::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	VERIFY (m_lstCheckDatabaseLoc.SubclassDlgItem (IDC_ALTERNATE_LOC_DATABASE, this));
	VERIFY (m_lstCheckWorkLoc.SubclassDlgItem (IDC_ALTERNATE_LOC_WORK, this));
	m_lstCheckWorkLoc.m_bSelectDisableItem = TRUE;
	
	if (m_bAlterDatabase)
	{
		m_lstCheckWorkLoc.ShowWindow(SW_SHOW);
		m_lstCheckDatabaseLoc.ShowWindow(SW_SHOW);
		m_cbWorkAlternate.ShowWindow(SW_SHOW);
		m_StaticDB.ShowWindow(SW_SHOW);
		m_StaticWK.ShowWindow(SW_SHOW);
		m_StaticText.ShowWindow(SW_HIDE);
	}
	else
	{
		m_lstCheckWorkLoc.ShowWindow(SW_HIDE);
		m_lstCheckDatabaseLoc.ShowWindow(SW_HIDE);
		m_cbWorkAlternate.ShowWindow(SW_HIDE);
		m_StaticDB.ShowWindow(SW_HIDE);
		m_StaticWK.ShowWindow(SW_HIDE);
		m_StaticText.ShowWindow(SW_SHOW);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgcreateDBAlternateLoc::OnWorkAlternate() 
{
	CString csCurentString,csTempo,csWork,csAuxiliary,csNewName;
	int index,nCheck;
	BOOL bEnable;
	
	csWork.LoadString (IDS_LOC_TYPE_WORK);
	csAuxiliary.LoadString (IDS__LOC_TYPE_AUXILIARY);
	
	if (csWork.IsEmpty() || csAuxiliary.IsEmpty())
		return;

	index = m_lstCheckWorkLoc.GetCurSel();
	if (index == LB_ERR)
		return;

	nCheck = m_lstCheckWorkLoc.GetCheck(index);
	if ( nCheck == 0 || nCheck == 2)
		return;

	m_lstCheckWorkLoc.GetText(index,csCurentString);
	if ( csCurentString.Find(csWork) == 0)
	{
		csTempo = csCurentString.Mid(csWork.GetLength()+1,csCurentString.GetLength());
		csNewName = csAuxiliary + _T(" ") + csTempo;
	}
	else if (csCurentString.Find(csAuxiliary)== 0)
	{
		csTempo = csCurentString.Mid(csAuxiliary.GetLength()+1,csCurentString.GetLength());
		csNewName = csWork + _T(" ") + csTempo;
	}
	else
		return;

	bEnable = m_lstCheckWorkLoc.IsItemEnabled(index);
	m_lstCheckWorkLoc.DeleteString(index);
	int nSel;
	nSel = m_lstCheckWorkLoc.InsertString(index,csNewName);
	if (nSel != LB_ERR)
	{
		m_lstCheckWorkLoc.SetCurSel (nSel);
		m_lstCheckWorkLoc.SetCheck (nSel,nCheck);
		m_lstCheckWorkLoc.EnableItem(nSel,bEnable);
	}
}

void CuDlgcreateDBAlternateLoc::OnCheckChange()
{
	CString csCurentString,csTempo,csWork,csAuxiliary;
	int nCheck,index;
	
	index = m_lstCheckWorkLoc.GetCaretIndex();
	if (index == LB_ERR)
		return;
	if (!m_lstCheckWorkLoc.IsItemEnabled(index))
	{
		m_lstCheckWorkLoc.SetCheck (index,TRUE);
		return;
	}
	csWork.LoadString(IDS_LOC_TYPE_WORK);
	csAuxiliary.LoadString(IDS__LOC_TYPE_AUXILIARY);

	m_lstCheckWorkLoc.GetText(index,csCurentString);
	nCheck = m_lstCheckWorkLoc.GetCheck(index);
	if ( nCheck == 1)
	{
		if ( csCurentString.Find(csWork) == -1) // not Found
			csTempo = csWork + _T(" ") + csCurentString;
		else
			csTempo = csCurentString;
	}
	else
	{
		if (csCurentString.Find(csWork) == 0) 
			csTempo = csCurentString.Mid(csWork.GetLength()+1, // +1 for whitespace
										 csCurentString.GetLength());
		else if (csCurentString.Find(csAuxiliary)== 0)
			csTempo = csCurentString.Mid(csAuxiliary.GetLength()+1,
										 csCurentString.GetLength());
		else
			csTempo = csCurentString;
	}
	m_lstCheckWorkLoc.DeleteString(index);
	int nSel;
	nSel = m_lstCheckWorkLoc.InsertString(index,csTempo);
	if (nSel != LB_ERR)
	{
		m_lstCheckWorkLoc.SetCaretIndex(nSel);
		m_lstCheckWorkLoc.SetCurSel (nSel);
		m_lstCheckWorkLoc.SetCheck (nSel,nCheck);
	}
}

void CuDlgcreateDBAlternateLoc::GetLocName(CString csCompleteStr, CString &csLocName)
{
	CString csWork,csAuxiliary,csDefault;
	int nTemp;

	csDefault.LoadString (IDS_DEFAULT);
	csWork.LoadString (IDS_LOC_TYPE_WORK);
	csAuxiliary.LoadString (IDS__LOC_TYPE_AUXILIARY);

	if (csDefault.IsEmpty() || csWork.IsEmpty() || csAuxiliary.IsEmpty())
		return;
	if ((nTemp = csCompleteStr.Find(csDefault)) != -1)
	{
		int nMax,nBegin;
		nBegin = csDefault.GetLength()+1+1+nTemp; // +1 for Whitesspace +1 for "("
		nMax = csCompleteStr.GetLength()- nBegin -1;// -1 for ")"
		csLocName = csCompleteStr.Mid(	nBegin,	nMax);
		return;
	}
	if (csCompleteStr.Find(csWork) == 0)
	{
		csLocName = csCompleteStr.Mid(csWork.GetLength()+1); // +1 for whitespace
		return;
	}
	if (csCompleteStr.Find(csAuxiliary) == 0)
	{
		csLocName = csCompleteStr.Mid(csAuxiliary.GetLength()+1);
		return;
	}
	csLocName = csCompleteStr;
}

void CuDlgcreateDBAlternateLoc::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CuDlgcreateDBAlternateLoc::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}
