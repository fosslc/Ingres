/****************************************************************************************
//                                                                                     //
//  Copyright (C) October, 1998 Computer Associates International, Inc.                //
//                                                                                     //
//    Source   : dgdpauus.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II/ VDBA (Installation Level Auditing).                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control                                                 //
//               The DOM Property, Audit all Users Page                                //
****************************************************************************************/
#ifndef DGDPAUUS_HEADER
#define DGDPAUUS_HEADER
#include "calsctrl.h"
#include "domilist.h"  // CuDomImageList
#include "multcol.h"

class CuDlgDomPropInstallationLevelAuditAllUsers : public CDialog
{
public:
	CuDlgDomPropInstallationLevelAuditAllUsers(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropInstallationLevelAuditAllUsers)
	enum { IDD = IDD_DOMPROP_INSTLEVEL_AUDITUSERS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CuDomImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropInstallationLevelAuditAllUsers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void AddItem (LPCTSTR lpszItem, CuNameOnly* pData);
	void DisplayItems();
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropInstallationLevelAuditAllUsers)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
