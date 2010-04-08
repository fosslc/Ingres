/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage2.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
**/

#if !defined (IAPPAGE2_HEADER)
#define IAPPAGE2_HEADER
#include "256bmp.h"
#include "fformat1.h"
#include "fformat2.h"


class CuPPage2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPPage2)

public:
	CuPPage2();
	~CuPPage2();

	// Dialog Data
	//{{AFX_DATA(CuPPage2)
	enum { IDD = IDD_PROPPAGE2 };
	CButton	m_cButtonShowRejectedSolution;
	CButton	m_cCheckEnforceFixedWidth;
	CTabCtrl	m_cTab1;
	C256bmp m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPPage2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation

protected:
	CString m_strPageTitle;
	int m_nFixedWidthCount;
	CWnd* m_pCurrentPage;
	CWnd* CreatePage(ImportedFileType fileType, CaSeparatorSet* pSet, CaSeparator* pSeparator, BOOL bFixedWidth = FALSE);

	void ResizeTab();
	void DisplayPage();
	void ConstructPagesFileFormat();
	void Loading();
	void EnforceFixedWidth(BOOL bAutoDetect = TRUE);

	// Generated message map functions
	//{{AFX_MSG(CuPPage2)
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnforceFixedWidth();
	afx_msg void OnDestroy();
	afx_msg void OnButtonShowRejectedSolution();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


#endif // IAPPAGE2_HEADER
