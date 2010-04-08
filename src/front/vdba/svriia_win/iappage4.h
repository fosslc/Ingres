/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage4.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 4 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
**/

#if !defined (IAPPAGE4_HEADER)
#define IAPPAGE4_HEADER
#include "calsctrl.h"
#include "256bmp.h"
#include "formdata.h"
#include "lsctlhug.h"

class CuPPage4 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPPage4)

public:
	CuPPage4();
	~CuPPage4();

	// Dialog Data
	//{{AFX_DATA(CuPPage4)
	enum { IDD = IDD_PROPPAGE4 };
	CEdit	m_cEditProgressCommitStep;
	CButton	m_cCheckProgressCommit;
	CButton	m_cButtonSave;
	CStatic	m_cStaticEmptyTable;
	CStatic	m_cStatic1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPPage4)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	class CaColumnFormat: public CObject
	{
	public:
		CaColumnFormat()
		{
			m_bDummy = FALSE;
			m_strName_Format = _T("");
			m_strNullOption = _T("");
		}
		virtual ~CaColumnFormat(){}

		CString m_strName_Format;
		CString m_strNullOption;
		BOOL m_bDummy;
	};

	HICON m_hIcon;
	CuListCtrlHuge m_listCtrlHuge;
	C256bmp m_bitmap;
	CString m_strNull;
	BOOL m_bTableIsEmpty;
	int  m_nAppendOrDelete; // Indicate that user checks "Append..." or "Replace ..."
	CString m_strPageTitle;

	BOOL ImportByCopy(CaIIAInfo* pData);
	//
	// int nMode =-1: ignore
	//           = 0: (fixed width)
	//           = 1: (special case, each line is considered as single column);
	//           = 2: (Do not generate the SKIP column syntax (dn).
	CString GenerateCopySyntax(TCHAR tchDelimiter, LPCTSTR lpszFile, int nMode =-1);
	BOOL CanCopyUseFileDirectly(
		CaSeparatorSet* pSet, 
		CaSeparator* pSep, 
		CTypedPtrList< CObList, CaDBObject* >& listColumn);
	void Loading();

	// Generated message map functions
	//{{AFX_MSG(CuPPage4)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnRadioAppend();
	afx_msg void OnRadioReplace();
	afx_msg void OnButtonSaveImportParameter();
	afx_msg void OnCheckProgressCommit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



#endif // IAPPAGE4_HEADER
