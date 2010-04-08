/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage3.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 3 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
**/

#if !defined (IAPPAGE3_HEADER)
#define IAPPAGE3_HEADER
#include "formdata.h"
#include "lschrows.h"
#include "256bmp.h"
#include "schkmark.h"

class CuPPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuPPage3)

public:
	CuPPage3();
	~CuPPage3();

	// Dialog Data
	//{{AFX_DATA(CuPPage3)
	enum { IDD = IDD_PROPPAGE3 };
	CComboBox	m_cComboNumericFormat;
	CComboBox	m_cComboDataFormat;
	CuStaticCheckMark	m_cStaticCheckMark;
	CStatic	m_cStaticTextNewTableInfo;
	CStatic	m_cStatic1;
	C256bmp m_bitmap;
	//}}AFX_DATA
	CuListCtrlDouble m_wndHeaderRows;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuPPage3)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CaColumn* m_pDummyColumn;
	CString m_strCreateTableDefaultType;
	int m_nLengthMargin;
	CString m_strPageTitle;
	BOOL m_bFirstActivate;    // TRUE if this page is the first time maded activate:
	UINT m_nCurrentDetection; // ID of the current detection
	ColumnSeparatorType m_nCurrentSeparatorType; 
	CaSeparatorSet* m_pSeparatorSet;
	CaSeparator* m_pSeparator;
	BOOL m_bCurrentFixedWidth;// Valid only if m_nCurrentSeparatorType = FMT_FIXEDWIDTH
	//
	// When the column is NULL, the detecttion returns 0 as length.
	// So use this member m_nLength4NullColum as the default length:
	int m_nLength4NullColum;

	void ShowRecords(CaIIAInfo* pData);
	BOOL ValidateData(CaIIAInfo* pData);
	void ComboDataFormat(CComboBox* pCombo);
	void ComboNumericFormat(CComboBox* pCombo);
	void Loading();

	// Generated message map functions
	//{{AFX_MSG(CuPPage3)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



#endif // IAPPAGE3_HEADER
