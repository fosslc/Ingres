/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIpDb.cpp, Implementation file.                                      //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control.                                                //              
//               The Database page.                                                    //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgipdb.h"
#include "resmfc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER	6

static void GetDisplayInfo ()
{
 
}

CuDlgIpmPageDatabases::CuDlgIpmPageDatabases(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageDatabases::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageDatabases)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageDatabases::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageDatabases)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(CuDlgIpmPageDatabases, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageDatabases)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageDatabases message handlers

void CuDlgIpmPageDatabases::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageDatabases::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LV_COLUMN lvcolumn;
	TCHAR strHeader[LAYOUT_NUMBER][32] = 
	{
		_T("External TX ID"),
		_T("Database"),
		_T("Session"),
		_T("Status"),
		_T("Write"),
		_T("Split")
	};
	int i, hWidth [LAYOUT_NUMBER] = {110 + 16, 65, 50, 100, 40, 40};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		//CString strHeader;
		//strHeader.LoadString (strHeaderID[i]);
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}

	m_ImageList.Create(IDB_TM_DB, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageDatabases::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageDatabases::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	ResetDisplay();
	return 0L;
}

void CuDlgIpmPageDatabases::ResetDisplay()
{

}
