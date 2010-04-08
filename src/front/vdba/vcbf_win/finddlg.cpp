/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : FindDlg.cpp : implementation file
**    Project  : OpenInges Configuration Manager 
**
** History:
**	22-nov-98 (cucjo01)
**	    Created
**	    Allows "Find" dialog for "History Of Changes" tab via F3 or CTRL-F
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "vcbf.h"
#include "FindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindDlg dialog


CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFindDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindDlg)
	DDX_Control(pDX, IDC_MATCH_CASE, m_cbMatchCase);
	DDX_Control(pDX, IDC_RB_UP, m_RadioUp);
	DDX_Control(pDX, IDC_RB_DOWN, m_RadioDown);
	DDX_Control(pDX, IDC_FIND_TEXT, m_FindText);
	DDX_Control(pDX, IDB_FIND_NEXT, m_FindNext);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
	//{{AFX_MSG_MAP(CFindDlg)
	ON_BN_CLICKED(IDB_FIND_NEXT, OnFindNext)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindDlg message handlers

void CFindDlg::OnFindNext()
{  bool bFound;
   int iFindLen, iTextLen, iPos;
   int iStartSel, iEndSel, iCursorStart, iCursorEnd;
   CString strText, strFind, strFindOrig, strBuffer;

   m_FindText.GetWindowText(strFind);
   m_EditText->GetWindowText(strText);
   strFindOrig=strFind;

   if (m_cbMatchCase.GetCheck() == 0)
   { strFind.MakeLower(); 
     strText.MakeLower();
   }
   
   iFindLen = strFind.GetLength();
   iTextLen = strText.GetLength();

   m_EditText->SetFocus();
   m_EditText->GetSel(iCursorStart, iCursorEnd);
   iStartSel=iCursorStart;
   iEndSel=iCursorEnd;
   bFound=FALSE;
   if (strFind.Compare(_T("")) == 0)
      goto UpdateField;

   if (m_RadioDown.GetCheck() == 1)
   {
#if defined (_MBCS) || defined (_DBCS)
		if (IsDBCSLeadByte ((BYTE)strText.GetAt (iCursorStart)))
		iCursorStart+= 2; // skip the trailing byte
#else
		iCursorStart += 1;
#endif
     strText = strText.Right(iTextLen - iCursorStart);
     iPos = strText.Find(strFind);

     if (iPos != -1)
     { bFound=TRUE;
       iStartSel = iPos + iCursorStart;
       iEndSel   = iPos + iCursorStart + iFindLen;
     }
   }
   else
   if (m_RadioUp.GetCheck() == 1)
   { iCursorStart -= 1;
#if defined (_MBCS) || defined (_DBCS)
		if ((iCursorStart-1 >= 0) && IsDBCSLeadByte ((BYTE)strText.GetAt (iCursorStart -1)))
			iCursorStart-= 1; // skip the trailing byte
#endif

     strText = strText.Left(iCursorStart);
     strText.MakeReverse();
     strFind.MakeReverse();
     iPos = strText.Find(strFind);
     
     if (iPos != -1)
     { bFound=TRUE;
       iStartSel = strText.GetLength() - iPos - iFindLen;
       iEndSel   = strText.GetLength() - iPos;
     }
   }

   UpdateField:
   if (bFound)
   { m_EditText->SetFocus();
     m_EditText->SetSel(iStartSel, iEndSel);
     m_EditText->UpdateData();
   }
   else
   { MessageBeep(MB_ICONEXCLAMATION);
     strBuffer.Format(IDS_ERR_NOT_FOUND, (LPCTSTR)strFindOrig);
     // if (m_cbMatchCase.GetCheck() == 1)
     // { strBuffer += "  Remember the case sensitive search is on!";
     // }
     AfxMessageBox(strBuffer);
   }
   
}

BOOL CFindDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_RadioDown.SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFindDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	m_EditText->SetFocus();
	CDialog::OnCancel();
}

void CFindDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
   int iCursorStart, iCursorEnd;
   CString strText, strBuffer;
 
   m_EditText->GetWindowText(strText);
 
   m_EditText->SetFocus();
   m_EditText->GetSel(iCursorStart, iCursorEnd);

   strBuffer = strText.Mid(iCursorStart, iCursorEnd-iCursorStart);
   strBuffer = strBuffer.Left(255);

   m_FindText.SetWindowText(strBuffer);
   m_FindText.SetFocus();
   m_FindText.SetSel(0, -1);
   CDialog::OnShowWindow(bShow, nStatus);
}
