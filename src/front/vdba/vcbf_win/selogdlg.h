/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : selogdlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//               Blatte Emmanuel (Custom Implemenations)                               //
//                                                                                     //
//    Purpose  : Modeless Dialog, Page (Log File) of Security                          //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/
#if !defined(SELOGDLG_HEADER)
#define SELOGDLG_HEADER

// selogdlg.h : header file
//
#include "editlsct.h"


class CuEditableListCtrlAuditLog : public CuEditableListCtrl
{
public:
	CuEditableListCtrlAuditLog();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlAuditLog();
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)        {return OnEditDlgOK(wParam, lParam);}      

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlAuditLog)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CuEditableListCtrlAuditLog)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};









/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityLogFile dialog

class CuDlgSecurityLogFile : public CDialog
{
public:
	CuDlgSecurityLogFile(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgSecurityLogFile)
	enum { IDD = IDD_SECURITY_PAGE_LOGFILE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgSecurityLogFile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	CuEditableListCtrlAuditLog m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgSecurityLogFile)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnButton1   (UINT wParam, LONG lParam);     // Not Used	
	afx_msg LONG OnButton2   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton3   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton4   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton5   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnUpdateData(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif
