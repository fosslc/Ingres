/****************************************************************************************
//                                                                                     //
//  Copyright (C) October, 1998 Computer Associates International, Inc.                //
//                                                                                     //
//    Source   : dgdpenle.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II/ VDBA (Installation Level Auditing).                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control                                                 //
//               The DOM PROP Enabled Level Page                                       //
****************************************************************************************/
#ifndef DGDPENLE_HEADER
#define DGDPENLE_HEADER
#include "calsctrl.h"
#include "domseri.h"



class CuDlgDomPropInstallationLevelEnabledLevel : public CDialog
{
public:
	CuDlgDomPropInstallationLevelEnabledLevel(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropInstallationLevelEnabledLevel)
	enum { IDD = IDD_DOMPROP_INSTLEVEL_ENABLEDLEVEL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CImageList m_ImageCheck;
	CImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropInstallationLevelEnabledLevel)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void AddItem (LPCTSTR lpszItem, CaAuditEnabledLevel* pData);
	void DisplayItems();
	void ResetDisplay();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropInstallationLevelEnabledLevel)
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

class CaInstallLevelSecurityState: public CObject
{
public:
	CaInstallLevelSecurityState(int nNodeHandle);
	virtual ~CaInstallLevelSecurityState();
	BOOL QueryState (CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel);
	BOOL UpdateState(CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel);

private:
	int m_nNodeHandle;
	int m_nSession;

};


#endif
