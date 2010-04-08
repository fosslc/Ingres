#ifndef DIALOG_TBL_JOURNALING_INCLUDED
#define DIALOG_TBL_JOURNALING_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dgtbjnl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgTableJournaling dialog

class CuDlgTableJournaling : public CDialog
{
// Construction
public:
	CuDlgTableJournaling(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CuDlgTableJournaling)
	enum { IDD = IDD_TBLJOURNALING };
	//}}AFX_DATA
  CString m_csTableName;
  CString m_csOwnerName;
  CString m_csVirtNodeName;
  CString m_csParentDbName;
  TCHAR   m_cJournaling;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgTableJournaling)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgTableJournaling)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRadJnlActive();
	afx_msg void OnRadJnlAfterckpdb();
	afx_msg void OnRadJnlInactive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
  BOOL ChangeTableJournaling(BOOL bActivate);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // DIALOG_TBL_JOURNALING_INCLUDED
