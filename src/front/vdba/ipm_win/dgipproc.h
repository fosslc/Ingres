/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipproc.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Process page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/


#ifndef DGIPPROC_HEADER
#define DGIPPROC_HEADER
#include "calsctrl.h"

class CuDlgIpmPageProcesses : public CDialog
{
public:
	CuDlgIpmPageProcesses(CWnd* pParent = NULL);  
	void AddProcess (
		LPCTSTR strID,
		LPCTSTR strPID, 
		LPCTSTR strType,
		LPCTSTR strOpenDB,
		LPCTSTR strWrite,
		LPCTSTR strForce,
		LPCTSTR strWait,
		LPCTSTR strBegin,
		LPCTSTR strEnd,
		void* pStruct);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageProcesses)
	enum { IDD = IDD_IPMPAGE_PROCESSES};
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageProcesses)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageProcesses)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData  (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
