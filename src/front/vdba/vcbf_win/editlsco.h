/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : Editlsco.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenInges Configuration Manager                                       //
//    Author   : Emmanuel Blattes - UK Sotheavut                                       //
//                                                                                     //
//                                                                                     //
//    Purpose  : list control customized for components list in cbf                    //
//               derived from CuEditableListCtrl                                       //
//                                                                                     //
****************************************************************************************/

//
// CuEditableListCtrlComponent window
#ifndef LYOUTCTL_COMPONENT_HEADER
#define LYOUTCTL_COMPONENT_HEADER

#include "editlsct.h" // base class

class CuEditableListCtrlComponent : public CuEditableListCtrl
{
private:
  void OnOpenCellEditor(UINT nFlags, CPoint point);

public:
	CuEditableListCtrlComponent();
	
  // virtual interface to afx_msg OnXyzDlgOK methods
  virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam);
	virtual LONG ManageEditSpinDlgOK (UINT wParam, LONG lParam);

	// Overrides
	//
	virtual ~CuEditableListCtrlComponent();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlComponent)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:

	//{{AFX_MSG(CuEditableListCtrlComponent)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditSpinDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};
#endif

