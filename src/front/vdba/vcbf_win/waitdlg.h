#if !defined(AFX_WAITDLG_H__50989AD3_6D34_11D2_9FE1_006008924264__INCLUDED_)
#define AFX_WAITDLG_H__50989AD3_6D34_11D2_9FE1_006008924264__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WaitDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

class CWaitDlg : public CDialog
{
// Construction
public:
    CWaitDlg(CString strText, UINT uiAVI_Resource, UINT uiDialogID = IDD_WAIT_DIALOG, CWnd* pParent = NULL);

    UINT m_uiAVI_Resource;
    CString m_strText;

// Dialog Data
	//{{AFX_DATA(CWaitDlg)
	enum { IDD = IDD_WAIT_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLG_H__50989AD3_6D34_11D2_9FE1_006008924264__INCLUDED_)
