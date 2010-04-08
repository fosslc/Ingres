/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : histdlg.cpp, Implementation file 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**               SCHALK P (Custom Implementation)
**    Purpose  : Modeless Dialog Page (History of Changes) 
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 21-nov-98 (cucjo01)
      Added support for "Find" dialog through F3 key or CTRL-F key combo
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
**
**/

#include "stdafx.h"
#include "vcbf.h"
#include "histdlg.h"
#include "wmusrmsg.h"
#include "ll.h"
#include "FindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static UINT WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);
CFindDlg dlgFind;
BOOL bFindInUse=FALSE;

/////////////////////////////////////////////////////////////////////////////
// CHistDlg dialog


CHistDlg::CHistDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHistDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHistDlg)
	m_str_hist = _T("");
	//}}AFX_DATA_INIT
}


void CHistDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHistDlg)
	DDX_Control(pDX, IDC_EDIT1, m_cstrhist);
	DDX_Text(pDX, IDC_EDIT1, m_str_hist);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHistDlg, CDialog)
	//{{AFX_MSG_MAP(CHistDlg)
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHistDlg message handlers

void CHistDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	if (IsWindow(m_cstrhist.m_hWnd)) 
	{
		CRect r;
		GetClientRect(r);
		r.top    += 10;
		r.bottom -= 10;
		r.left	 += 10;
		r.right  -= 10;
		m_cstrhist.MoveWindow(r);
	}
}

BOOL CHistDlg::OnInitDialog() 
{	CDialog::OnInitDialog();
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHistDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


LONG CHistDlg::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	LPTSTR szBufName;
	CWaitCursor wait;    // display wait cursor
	try
	{
		if ((szBufName =(LPTSTR) VCBFllLoadHist ( )) == NULL) {
			AfxMessageBox (IDS_ERR_LOAD_HISTORY);
			CString csTemp;
			csTemp.LoadString(IDS_ERR_LOAD_HISTORY);
			m_cstrhist.SetWindowText(csTemp);
		}
		else {
			m_cstrhist.SetWindowText((LPCTSTR)szBufName);
			m_cstrhist.LineScroll(m_cstrhist.GetLineCount( ));
			VCBFllFreeString( szBufName );
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CHistDlg::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}

	return 0L;
}

BOOL CHistDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
    	if ( (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
		     (pMsg->wParam == VK_F3) || 
             ((pMsg->wParam == 0x46) && (GetKeyState(VK_CONTROL) & ~1) != 0) )
		{
            dlgFind.m_EditText=&m_cstrhist;
            if (!bFindInUse)
            { dlgFind.Create(IDD_FIND_DIALOG);
              dlgFind.CenterWindow();
              bFindInUse=TRUE;
            }
            dlgFind.BringWindowToTop();
            dlgFind.ShowWindow(SW_SHOW);
            return TRUE;
		}

	return CDialog::PreTranslateMessage(pMsg);
}

