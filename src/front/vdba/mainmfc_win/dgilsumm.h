/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgILSumm.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Detail (Summary) page of Locations.            //              
//                                                                                     //
****************************************************************************************/
#ifndef DGILSUMM_HEADER
#define DGILSUMM_HEADER
#include "statfrm.h"
#include "statlege.h"

class CuDlgIpmSummaryLocations : public CDialog
{
public:
	CuDlgIpmSummaryLocations(CWnd* pParent = NULL);  
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmSummaryLocations)
	enum { IDD = IDD_IPMDETAIL_LOCATIONS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	CuStatisticBarLegendList  m_cListLegend;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmSummaryLocations)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuStatisticBarFrame* m_pDlgFrame;
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmSummaryLocations)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnApply      (WPARAM wParam, LPARAM lParam);
};
#endif
