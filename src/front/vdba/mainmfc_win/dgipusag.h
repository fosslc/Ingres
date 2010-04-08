/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIpUsag.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control                                                 //              
//               The Location usage page.                                              //
****************************************************************************************/
#ifndef DGIPUSAG_HEADER
#define DGIPUSAG_HEADER
extern "C"
{
#include "dba.h"
#include "dbaset.h"
}
class CuDlgIpmPageLocationUsage : public CFormView
{
protected:
	CuDlgIpmPageLocationUsage();
	DECLARE_DYNCREATE(CuDlgIpmPageLocationUsage)

public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmPageLocationUsage)
	enum { IDD = IDD_IPMPAGE_LOCATION_USAGE };
	CString m_strName;
	CString m_strArea;
	BOOL	m_bDatabase;
	BOOL	m_bWork;
	BOOL	m_bJournal;
	BOOL	m_bCheckpoint;
	BOOL	m_bDump;
	//}}AFX_DATA
	LOCATIONPARAMS m_lcusageStruct;


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageLocationUsage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmPageLocationUsage();
	void ResetDisplay();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageLocationUsage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
