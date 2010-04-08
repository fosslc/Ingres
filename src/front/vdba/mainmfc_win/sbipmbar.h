/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sbipmbar.h, Header file
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
** 
*    Purpose  : IPM Dialog Bar. When the control bar is floating, 
**              its size is the vertical dialog bar template.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 21-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#ifndef SBIPMBAR_HEADER
#define SBIPMBAR_HEADER
#include "bmpbtn.h"

class CuIpmDlgBar : public CDialogBar
{
	CWnd* m_pParent;
public:
	CuIpmDlgBar();
	~CuIpmDlgBar();
	BOOL Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	BOOL Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	void UpdateDisplay();

	CButton* GetNullResourceButton()        {return &m_Check1;}
	CButton* GetInternalSessionButton()     {return &m_Check2;}
	CButton* GetSysLockListButton()         {return &m_Check3;}
	CButton* GetInactiveTransactionButton() {return &m_Check4;}
	CComboBox* GetComboBoxResourceType()    {return &m_ComboRCType;}
	//
	// Overrides
	virtual CSize CalcDynamicLayout (int nLength, DWORD dwMode);
	void SetContextDriven(BOOL bContext){m_bContextDriven = bContext;}

private:
	CRect ctrlVRect [15]; // The control's placement.
	CRect ctrlHRect [15]; // The control's placement.
	BOOL  m_bContextDriven;
	HICON m_arrayIcon[6];

protected:
	CSize     m_HdlgSize;      // Control bar size when it is Horizontal docked.
	CSize     m_VdlgSize;      // Control bar size when it is Vertical docked.  
	CPoint    m_Button01Position;
	CComboBox m_ComboRCType;   // Resource Type
	CButton   m_Check1;        // NULL Resource
	CButton   m_Check2;        // Internal Session
	CButton   m_Check3;        // System Lock List
	CButton   m_Check4;        // Inactive Transaction

	CuBitmapButton   m_Button01;      // Refresh
	CuBitmapButton   m_Button02;      // Shutdown
	CuBitmapButton   m_Button03;      // Expand branch
	CuBitmapButton   m_Button04;      // Expand all
	CuBitmapButton   m_Button05;      // Collapse branch
	CuBitmapButton   m_Button06;      // Collapse all

	CButton   m_Check5;        // Express Refresh
	CEdit     m_cEditFrequency;// Frequency 
	CButton   m_Check6;        // Only if ...

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

//
// CuIpmBarDlgTemplate dialog

class CuIpmBarDlgTemplate : public CDialog
{
public:
	CuIpmBarDlgTemplate(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuIpmBarDlgTemplate)
	enum { IDD = IDD_IPMBARV };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIpmBarDlgTemplate)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuIpmBarDlgTemplate)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
