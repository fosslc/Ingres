/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edbcfram.h : header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame.
**
** History:
**
** 09-Oct-2003 (uk$so01)
**    Created
**    SIR #106648, (EDBC Specific)
*/

#if !defined(AFX_EDBCFRAM_H__9156E161_F623_46F9_A45E_FF4CB66DA178__INCLUDED_)
#define AFX_EDBCFRAM_H__9156E161_F623_46F9_A45E_FF4CB66DA178__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "mainfrm.h"

class CdSqlQueryEdbc;
class CfMainFrameEdbc : public CfMainFrame
{
	DECLARE_DYNCREATE(CfMainFrameEdbc)
protected:
	CfMainFrameEdbc();

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrameEdbc)
	//}}AFX_VIRTUAL

	// Implementation
	virtual ~CfMainFrameEdbc();

protected:
	virtual BOOL SetConnection(BOOL bDisplayMessage = TRUE);
	virtual UINT HandleCommandLine(CdSqlQuery* pDoc);

protected:
	// Generated message map functions
	//{{AFX_MSG(CfMainFrameEdbc)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDBCFRAM_H__9156E161_F623_46F9_A45E_FF4CB66DA178__INCLUDED_)
