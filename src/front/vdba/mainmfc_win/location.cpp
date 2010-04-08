// location.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "location.h"
#include "createdb.h"
extern "C" {
#include "dbaset.h"
#include "dll.h"
#include "main.h"
#include "domdata.h"
#include "getdata.h"
#include "dlgres.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBLocation dialog


CuDlgcreateDBLocation::CuDlgcreateDBLocation(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgcreateDBLocation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgcreateDBLocation)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgcreateDBLocation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgcreateDBLocation)
	DDX_Control(pDX, IDC_LOC_WORK, m_ctrlLocWork);
	DDX_Control(pDX, IDC_LOC_JOURNAL, m_ctrlLocJournal);
	DDX_Control(pDX, IDC_LOC_DUMP, m_ctrlLocDump);
	DDX_Control(pDX, IDC_LOC_DATABASE, m_ctrlLocDatabase);
	DDX_Control(pDX, IDC_LOC_CHECKPOINT, m_ctrlLocCheckPoint);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgcreateDBLocation, CDialog)
	//{{AFX_MSG_MAP(CuDlgcreateDBLocation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBLocation message handlers

void CuDlgcreateDBLocation::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgcreateDBLocation::OnInitDialog() 
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgcreateDBLocation::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CuDlgcreateDBLocation::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialog)));
   CxDlgCreateAlterDatabase* pParentDlg = (CxDlgCreateAlterDatabase*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}
