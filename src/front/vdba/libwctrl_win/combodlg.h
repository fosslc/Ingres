/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : combodlg.h Header File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Special ComboBox for Owner draw List Control(Editable)
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 17-Nov-2000: (uk$so01) 
**    Add combobox (dropdown style, editable)
**    When using this control, just consider that you have two comboboxes.
**    one is dropdowlist, and another is dropdown.
**    Because ComboBox has a 'pre-hWnd' and you can only set this style
**    at the design-time.
**
**/


#ifndef COMBODLG_HEADER
#define COMBODLG_HEADER
#include "rcdepend.h"

class CuLayoutComboDlg : public CDialog
{
public:
	CuLayoutComboDlg(CWnd* pParent = NULL);   // standard constructor
	CuLayoutComboDlg(CWnd* pParent, CString strText, int iItem, int iSubItem);  
	CString GetText ();
	void SetData    (LPCTSTR lpszText, int iItem, int iSubItem);
	void GetText    (CString& strText);
	void GetEditItem(int& iItem, int& iSubItem){iItem = m_iItem; iSubItem = m_iSubItem;};
	void SetFocus();
	void virtual OnOK();
	void OnCancel();
	int AddItem    (LPCTSTR lpszItem);
	int SetItemData(int nIndex, DWORD dwItemData);
	DWORD GetItemData(int nIndex);
	void SetMode(BOOL bEditable = FALSE)
	{
		//
		// Default mode m_bEditable = FALSE;
		m_bEditable = bEditable;
		ChangeMode(m_bEditable);
	}
	BOOL IsEditableMode(){return m_bEditable;}
	CComboBox* GetComboBox();

	// Dialog Data
	//{{AFX_DATA(CuLayoutComboDlg)
	enum { IDD = IDD_LAYOUTCTRLCOMBO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuLayoutComboDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

protected:
	CString m_StrText;
	int     m_iItem;
	int     m_iSubItem;
	BOOL    m_bEditable;

	void ChangeMode(BOOL bEditable);
	// Generated message map functions
	//{{AFX_MSG(CuLayoutComboDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	afx_msg void OnEditchangeCombo2();
	DECLARE_MESSAGE_MAP()
};
#endif
