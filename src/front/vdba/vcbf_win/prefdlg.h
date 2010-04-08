/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : prefdlg.h, header file                                                //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               SCHALK P (Custom Implementation)                                      //
//                                                                                     //
//    Purpose  : Modeless Dialog Page (Preference)                                     //
****************************************************************************************/
/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog
#if !defined (PREFDLG_HEADER)
#define PREFDLG_HEADER

class CPrefDlg : public CDialog
{
public:
	CPrefDlg(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;};
	void OnCancel() {return;};

	// Dialog Data
	//{{AFX_DATA(CPrefDlg)
	enum { IDD = IDD_PREFERENCES };
	CButton	m_CButtonPreference;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrefDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrefDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckPreference();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif
