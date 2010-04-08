/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : histdlg.h, header file                                                //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               SCHALK P (Custom Implementation)                                      //
//                                                                                     //
//    Purpose  : Modeless Dialog Page (History of Changes)                             //
****************************************************************************************/
#if !defined (HISTDLG_HEADER)
#define HISTDLG_HEADER
/////////////////////////////////////////////////////////////////////////////
// CHistDlg dialog

class CHistDlg : public CDialog
{
// Construction
public:
	CHistDlg(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;};
	void OnCancel() {return;};


// Dialog Data
	//{{AFX_DATA(CHistDlg)
	enum { IDD = IDD_HISTORY };
	CEdit	m_cstrhist;
	CString	m_str_hist;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHistDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHistDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};
	
#endif
