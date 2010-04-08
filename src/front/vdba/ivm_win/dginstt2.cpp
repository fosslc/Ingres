/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : dginstt2.cpp , Implementation File
**
**  Project  : Ingres II / Visual Manager
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Status Page for Installation branch  (Tab 2)
**
**  History:
**	09-Feb-1999 (uk$so01)
**	    Created.
**	11-Feb-2000 (somsa01)
**	    Changed OnUpdateData() to selectively update from the ingstart.log
**	    only new lines (or, refresh the entire window if the size has
**	    decreased from out last known size.
**	01-Mar-2000 (uk$so01)
**	    SIR #100635, Add the Functionality of Find operation.
**	05-Jul-2000 (uk$so01)
**	    BUG #102010, Do not open file using ifstream in
**	    (theApp.m_strLoggedStartStop); if it is not modified.
**	06-Jul-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	20-Jun-2001 (noifr01)
**	    (bug 105065) the criteria for incremental refresh is done on
**	    the string that had been read the previous time, rather than
**	    only on the file size
**	04-Jun-2002 (hanje04)
**	     Define tchszReturn to be 0x0a 0x00 for MAINWIN builds to stop
**	     unwanted ^M's in generated text files.
**	27-aug-2002 (somsa01)
**	    Corrected prior cross-integration.
**	02-Feb-2004 (noifr01)
**	    Removed any unneeded reference to iostream libraries, including now
**	    unused internal test code 
**	02-Feb-2004 (somsa01)
**	    Updated to build with .NET 2003 compiler.
**	19-Mar-2009 (drivi01)
**          Move fstream includes to stdafx.h.
*/

#include "stdafx.h"
#include "ivm.h"
#include "dginstt2.h"
#include "wmusrmsg.h"
#include "ivmdml.h"
#include "findinfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstallationTab2 dialog


CuDlgStatusInstallationTab2::CuDlgStatusInstallationTab2(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusInstallationTab2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusInstallationTab2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_fileStatus.m_mtime = 0;
	m_csCurOutput = _T("");

}


void CuDlgStatusInstallationTab2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusInstallationTab2)
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusInstallationTab2, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusInstallationTab2)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()


static BOOL IVM_IsOpen (ifstream& f)
{
	#if defined (MAINWIN)
	return (f)? TRUE: FALSE;
	#else
	return f.is_open();
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstallationTab2 message handlers

void CuDlgStatusInstallationTab2::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgStatusInstallationTab2::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cEdit1.MoveWindow (r);
}

LONG CuDlgStatusInstallationTab2::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	TCHAR tchszText [512];
#ifdef MAINWIN
	TCHAR tchszReturn [] = {0x0A, 0x00};
#else
	TCHAR tchszReturn [] = {0x0D, 0x0A, 0x00};
#endif

	try
	{
		//
		// Check to see if the file has been modified since
		// the last access:
		BOOL bModified = TRUE;
		CFileStatus rStatus;
		if (CFile::GetStatus(theApp.m_strLoggedStartStop, rStatus))
		{
			if (m_fileStatus.m_mtime == rStatus.m_mtime)
				bModified = FALSE;
			m_fileStatus.m_mtime = rStatus.m_mtime;
		}
		if (!bModified)
			return 0;
		
		ifstream in (theApp.m_strLoggedStartStop);
		if (!IVM_IsOpen (in))
			return 0;

		int nLen;
		CString strText = _T("");
		while (in.peek() != EOF)
		{
			in.getline(tchszText, 510);
			strText += tchszText;
			strText += tchszReturn;
		}
		if (strText.Find((LPCTSTR)m_csCurOutput)==0) { // begin of file hasn't changed
			int prevlen = m_csCurOutput.GetLength();
			CString csAddedString = strText.Mid(prevlen);

			nLen = m_cEdit1.GetWindowTextLength();
			m_cEdit1.SetSel (nLen, nLen);
			m_cEdit1.ReplaceSel(csAddedString);
		}
		else { // the begin of the file has changed. refresh all
			m_cEdit1.Clear();
			m_cEdit1.SetWindowText(_T(""));
			nLen = m_cEdit1.GetWindowTextLength();
			m_cEdit1.SetSel (nLen, nLen);
			m_cEdit1.ReplaceSel(strText);
		}
		m_csCurOutput = strText;

		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuDlgStatusInstallationTab2::OnUpdateData, fail to refresh data"));
	}

	return 0L;
}


BOOL CuDlgStatusInstallationTab2::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cEdit1.LimitText (0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LONG CuDlgStatusInstallationTab2::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStatusInstallationTab2::OnFind \n");
	return FIND_EditText ((WPARAM)&m_cEdit1, lParam);
}
