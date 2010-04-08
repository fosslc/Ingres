/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : waitdlgprogress.h, Header File
**    Project  : OpenIngres Configuration Manager
**    Author   : Joseph Cuccia 
**    Purpose  : Displays progress bar during creation of log files 
**               See the CONCEPT.DOC for more detail
**
** History:
**
** 02-Dec-1998 (cucjo01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
**/

#if !defined(AFX_WAITDLGPROGRESS_H__FB97BF11_83BC_11D2_9FE5_006008924264__INCLUDED_)
#define AFX_WAITDLGPROGRESS_H__FB97BF11_83BC_11D2_9FE5_006008924264__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WaitDlgProgress.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitDlgProgress dialog

class CWaitDlgProgress : public CDialog
{
// Construction
public:
    CWaitDlgProgress(CString strText, UINT uiAVI_Resource, UINT uiDialogID = IDD_WAIT_DIALOG_PROGRESS, CWnd* pParent = NULL);
    void StepProgressBar();
    void SetText(LPCTSTR szText);

    UINT m_uiAVI_Resource;
    CString m_strText;

// Dialog Data
	//{{AFX_DATA(CWaitDlgProgress)
	enum { IDD = IDD_WAIT_DIALOG_PROGRESS };
	CAnimateCtrl	m_Avi;
	CProgressCtrl	m_ProgressBar;
	CString	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitDlgProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitDlgProgress)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLGPROGRESS_H__FB97BF11_83BC_11D2_9FE5_006008924264__INCLUDED_)
