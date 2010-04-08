/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgUserPriv.h
** 
** History:
**	08-Feb-2010 (drivi01)
**		Created.
**/

#if !defined(AFX_XDLGUSERPRIV__INCLUDED_)
#define AFX_XDLGUSERPRIV__INCLUDED_

#pragma once

#include "resmfc.h"
#include "urpectrl.h"
// CxDlgUserPriv dialog

class CxDlgUserPriv : public CDialogEx
{
	DECLARE_DYNAMIC(CxDlgUserPriv)

public:
	CxDlgUserPriv(CWnd* pParent = NULL);   // standard constructor
	virtual ~CxDlgUserPriv();
	void InitClassMember(BOOL bInitMember = TRUE);

// Dialog Data
	enum { IDD = IDD_XUSER_PRIV };

protected:
	CImageList m_ImageCheck;
	CuEditableListCtrlURPPrivileges m_cListPrivilege;
	
	void FillPrivileges();
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg BOOL OnInitDialog();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	DECLARE_MESSAGE_MAP()

private:
	

};

#endif
