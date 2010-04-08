/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIpDb.h, Header file.                                                //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control.                                                //              
//               The Database page.                                                    //
****************************************************************************************/
#ifndef DGIPDB_HEADER
#define DGIPDB_HEADER
#include "calsctrl.h"

#include "domilist.h"

class CuDlgIpmPageDatabases : public CDialog
{
public:
	CuDlgIpmPageDatabases(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageDatabases)
	enum { IDD = IDD_IPMPAGE_DATABASES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;
	CuDomImageList m_ImageList;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageDatabases)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageDatabases)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam = 0, LPARAM lParam = 0L);
	DECLARE_MESSAGE_MAP()
};
#endif
