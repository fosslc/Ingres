/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgpistre.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes - inspired from Sotheavut's code
**    Purpose  : Page of Table control: Display list of replications 
**               when tree selection is on static item "Replication"
**
** History:
**
** xx-Sep-1998 (Emmanuel Blattes)
**    Created
*/


#ifndef DGIPSTRE_HEADER
#define DGIPSTRE_HEADER
#include "calsctrl.h"

class CdIpmDoc;
class CuDlgIpmPageStaticReplications : public CDialog
{
public:
	CuDlgIpmPageStaticReplications(CWnd* pParent = NULL);  
	void AddReplication (
		LPCTSTR strName, 
		int imageIndex,
		void* pStruct);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageStaticReplications)
	enum { IDD = IDD_IPMPAGE_STATIC_SERVERS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageStaticReplications)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	void GetDisplayInfo (CdIpmDoc* pDoc, LPRESOURCEDATAMIN pData, CString& strName, int& imageIndex);
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageStaticReplications)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	afx_msg long OnPropertiesChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
