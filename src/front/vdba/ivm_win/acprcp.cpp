/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : acprcp.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Archiver and Recover Log events
**
**  History:
**	05-Jul-1999 (uk$so01)
**	    Created
**	01-Mar-2000 (uk$so01)
**	    SIR #100635, Add the Functionality of Find operation.
**	31-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	04-Jun-2002 (hanje04)
**	    Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**	    unwanted ^M's in generated text files.
**	27-aug-2002 (somsa01)
**	    Corrected errors in cross-integration.
** 02-Feb-2004 (noifr01)
**    removed any unneeded reference to iostream libraries, including now
**    unused internal test code 
*/


#include "stdafx.h"
#include "ivm.h"
#include "ivmdisp.h"
#include "acprcp.h"
#include "wmusrmsg.h"
#include "getinst.h"
#include "findinfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgArchiverRecoverLog dialog


CuDlgArchiverRecoverLog::CuDlgArchiverRecoverLog(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgArchiverRecoverLog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgArchiverRecoverLog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgArchiverRecoverLog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgArchiverRecoverLog)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgArchiverRecoverLog, CDialog)
	//{{AFX_MSG_MAP(CuDlgArchiverRecoverLog)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgArchiverRecoverLog message handlers

void CuDlgArchiverRecoverLog::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgArchiverRecoverLog::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cEdit1.MoveWindow (r);
}

LONG CuDlgArchiverRecoverLog::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TCHAR tchszText [512];
#ifdef MAINWIN
	TCHAR tchszReturn [] = {0x0D, 0x0A, 0x00};
#else
	TCHAR tchszReturn [] = {0x0A, 0x00};
#endif

	try
	{
		tchszText[0] = _T('\0');
		m_cEdit1.Clear();
		m_cEdit1.SetWindowText(_T(""));
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		ASSERT (pPageInfo);
		if (!pPageInfo)
			return 0;
		if (pPageInfo->GetClassName().CompareNoCase (_T("ARCHIVER")) == 0) {
			FileContentBuffer FileContent((char *) (LPCTSTR) theApp.m_strLoggedArchiver,200000L,TRUE);
			m_cEdit1.SetWindowText((LPCTSTR)FileContent.GetBuffer());
			return 0L;
		}
		else
		if (pPageInfo->GetClassName().CompareNoCase (_T("RECOVERY")) == 0) {
			FileContentBuffer FileContent((char *) (LPCTSTR) theApp.m_strLoggedRecovery,200000L,TRUE);
			m_cEdit1.SetWindowText((LPCTSTR)FileContent.GetBuffer());
			return 0L;
		}
		else
			return 0;

		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuDlgArchiverRecoverLog::OnUpdateData, fail to refresh data"));
	}
	return 0L;
}

BOOL CuDlgArchiverRecoverLog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cEdit1.LimitText (0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LONG CuDlgArchiverRecoverLog::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LONG CuDlgArchiverRecoverLog::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgArchiverRecoverLog::OnFind \n");
	return FIND_EditText((WPARAM)&m_cEdit1, lParam);
}
