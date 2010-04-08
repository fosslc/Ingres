/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage1.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 1 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 10-Mar-2010 (drivi01) SIR 123397
**    Control "Specify Custom Separator" option with a checkbox.
**    Add routines for checking empty files.
**    Switch "First Line Header" option with "Skip lines" options,
**    allow header option to set skip lines at 1.  If skip lines is
**    increased to 2 or above, the headers become unchecked as we're 
**    no longer just skipping headers.
**/

#if !defined (IAPPAGE1_HEADER)
#define IAPPAGE1_HEADER
#include "formdata.h"
#include "256bmp.h"


class CuPPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPPage1)

public:
	CuPPage1();
	~CuPPage1();

	// Dialog Data
	//{{AFX_DATA(CuPPage1)
	enum { IDD = IDD_PROPPAGE1 };
	CButton	m_cCheckFirstLineHeader;
	CButton	m_cButton3Periods;
	CButton	m_cButtonLoad;
	CEdit	m_cEditLineCountToSkip;
	CSpinButtonCtrl	m_cSpin1;
	CButton	m_cCheckMatchNumberAndOrder;
	CEdit	m_cEditMoreSeparator;
	CEdit	m_cEditKB2Scan;
	CStatic	m_cStaticFileSize;
	CStatic	m_cStatic1;
	CEdit	m_cEditTable2Create;
	CEdit	m_cEditFile2BeImported;
	CTreeCtrl	m_cTreeIgresTable;
	C256bmp m_bitmap;
	CButton m_cCustomSeparator;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPPage1)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_imageList;
	HICON m_hIcon;
	CFileStatus* m_pStatusFile2BeImported;
	CFileStatus* m_pStatusFileParameter;
	int m_nCurrentCheckMatchFileAndTable;
	CString m_strPageTitle;

	BOOL m_bAllowExpandTree;
	BOOL m_bAllowSelChangeTree;
	BOOL m_bInterrupted; // The detection has been interrupted ?
	//
	// Check if all mendatory input parameters have been provided:
	BOOL CanGoNext();
	//
	// When user clicks on the next button, this funtion will tell
	// if the detection is to be performed or not:
	BOOL NeedToDetectFormat();

	//
	// When user chooses the DBF file, the OEM will be selected automatically:
	void PreSelectAnsiOem(LPCTSTR lpszFile);
	void CollectData(CaIIAInfo* pData, BOOL bNeed2Detect);
	void Loading(LPCTSTR lpszFile);

	// Generated message map functions
	//{{AFX_MSG(CuPPage1)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemexpandingTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonParameterFile();
	afx_msg void OnButton2ImportFile();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangedTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckFileMatchTable();
	afx_msg void OnKillfocusEditFile2BeImported();
	afx_msg void OnSelchangingTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditSkipLine();
	afx_msg void OnCheckFirstLineHeader();
	afx_msg void OnButtonCustomSeparator();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


#endif // IAPPAGE1_HEADER
