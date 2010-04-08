/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : editlsdp.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Special Owner draw List Control for the Generic Derived Page          //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/

#ifndef EDITLISTCONTROLGENERICDERIVED_HEADER
#define EDITLISTCONTROLGENERICDERIVED_HEADER
#include "editlsgn.h"


class CuEditableListCtrlGenericDerived : public CuEditableListCtrlGeneric
{
public:
	CuEditableListCtrlGenericDerived();
	void EditValue (int iItem, int iSubItem, CRect rcCell);

	// Overrides
	//
	virtual ~CuEditableListCtrlGenericDerived();
	void SetGenericForCache (BOOL bCache = TRUE){m_bCache = bCache;};
	BOOL GetGenericForCache (){return m_bCache;};

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlGenericDerived)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam)  {return OnEditNumberDlgOK(wParam, lParam);}
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam)    {return OnEditSpinDlgOK(wParam, lParam);}  
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);

	// Generated message map functions
protected:
	BOOL m_bCache; // When this control is used for CACHE, the low-level call is slightly
                   // different. If this member is TRUE, then the control is used for CACHE.

	//{{AFX_MSG(CuEditableListCtrlGenericDerived)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
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

