/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : LayEdDlg.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Edit Box to be used with Editable Owner Draw ListCtrl                 //
****************************************************************************************/
/* History:
* --------
* uk$so01: (Sept-1997 created)
*
*
*/

#ifndef LAYEDDLG_HEADER
#define LAYEDDLG_HEADER
#include "peditctl.h"   // Special Parse Edit

class CuLayoutEditDlg : public CDialog
{
public:
	CuLayoutEditDlg(CWnd* pParent = NULL);
	CuLayoutEditDlg(CWnd* pParent, CString strText, int iItem, int iSubItem);  
	CString GetText ();
	void SetData    (LPCTSTR lpszText, int iItem, int iSubItem);
	void GetText    (CString& strText);
	void GetEditItem(int& iItem, int& iSubItem){iItem = m_iItem; iSubItem = m_iSubItem;};
	void SetExpressionDialog (BOOL bExpressionDld = TRUE){m_bUseExpressDialog = bExpressionDld;};
	void SetFocus();
	void OnOK();
	void OnCancel();
	void SetReadOnly (BOOL bReadOnly = FALSE);

	void SetSpecialParse (BOOL bSpecialParse = TRUE){m_bUseSpecialParse = bSpecialParse;}
	void SetParseStyle   (WORD wParseStyle   = PES_INTEGER){m_wParseStyle = wParseStyle;}

	// Dialog Data
	//{{AFX_DATA(CuLayoutEditDlg)
	enum { IDD = IDD_LAYOUTCTRLEDIT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
protected:
	void ResizeControls();

	WORD m_wParseStyle;
	BOOL m_bUseSpecialParse;
	CuParseEditCtrl m_cEdit1;


	CString m_strItemText;
	int     m_iItem;
	int     m_iSubItem;
	BOOL    m_bUseExpressDialog;
	BOOL    m_bReadOnly;
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuLayoutEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
	
protected:
	// Generated message map functions
	//{{AFX_MSG(CuLayoutEditDlg)
	afx_msg void OnButtonAssistant();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
