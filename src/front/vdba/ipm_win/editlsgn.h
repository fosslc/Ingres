/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : editlsgn.h, Header File
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Special Owner draw List Control(Editable)
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created
*/

#ifndef EDITLISTCONTROLSTARTUPSETTING_HEADER
#define EDITLISTCONTROLSTARTUPSETTING_HEADER
#include "editlsct.h"


class CuEditableListCtrlStartupSetting : public CuEditableListCtrl
{
public:
	CuEditableListCtrlStartupSetting();
	void EditValue (int iItem, int iSubItem, CRect rcCell);
	// Overrides
	//
	virtual ~CuEditableListCtrlStartupSetting();

	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam)  {return OnEditNumberDlgOK(wParam, lParam);}
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam)    {return OnEditSpinDlgOK(wParam, lParam);}  
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlStartupSetting)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:

	//{{AFX_MSG(CuEditableListCtrlStartupSetting)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgCancel (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgCancel (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};
#endif

