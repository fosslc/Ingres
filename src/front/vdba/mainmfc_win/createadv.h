/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  createadv.h
** 
** History:
**	08-Feb-2010 (drivi01)
**		Created.
**/

#if !defined(AFX_CREATEDB_ADV__INCLUDED_)
#define AFX_CREATEDB_ADV__INCLUDED_

#pragma once
#include "stdafx.h"
#include "uchklbox.h"
#include "resmfc.h"
#include "createdb.h"
extern "C" {
	#include "dbaset.h"
}


// CuDlgcreateDBAdvanced

class CuDlgcreateDBAdvanced : public CDialogEx
{
public:
	CuDlgcreateDBAdvanced(CWnd* pParent = NULL);
	virtual ~CuDlgcreateDBAdvanced();
	void UpdateCoordName();


	// Dialog Data
	//{{AFX_DATA(CuDlgcreateDBAdvanced)
	enum { IDD = IDD_CREATEDB_ADVANCED };
	CButton	m_cbReadOnly;
	CStatic	m_ctrlStaticCoordName;
	CButton		m_ctrlPublic;
	CButton	m_ctrlGenerateFrontEnd;
	CButton	m_ctrlDistributed;
	CEdit m_ctrlLanguage;
	CEdit m_ctrlCoordName;
	CuCheckListBox m_CheckFrontEndList;
	CComboBox m_CtrlCatalogsPageSize;
	CStatic m_CtrlStaticCatPageSize;
	//}}AFX_DATA

	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgcreateDBAdvanced)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		afx_msg void OnDistributed();
		afx_msg void OnGenerateFrontEnd();
		afx_msg void OnReadOnly();
		afx_msg void OnOK();
		afx_msg void OnCancel();
	//}}AFX_VIRTUAL

		//CxDlgCreateAlterDatabase* pParentDlg;

	private:
		void InitialiseEditControls();
		void InitPageSize (LONG lPage_size);
		BOOL OccupyProductsControl();
	DECLARE_MESSAGE_MAP()
};

#endif 

