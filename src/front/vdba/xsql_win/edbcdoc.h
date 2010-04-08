/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : edbcdoc.h : header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : interface of the CdSqlQueryEdbc class
**
** History:
**
** 09-Oct-2003 (uk$so01)
**    SIR #106648, (EDBC Specific).
*/


#if !defined(AFX_EDBCDOC_H__73AD8B10_9005_4334_AAFA_9D7187B0F6F7__INCLUDED_)
#define AFX_EDBCDOC_H__73AD8B10_9005_4334_AAFA_9D7187B0F6F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "xsqldoc.h"
class CuSqlqueryEdbc;
class CdSqlQueryEdbc : public CdSqlQuery
{
protected:
	CdSqlQueryEdbc(); 
	DECLARE_DYNCREATE(CdSqlQueryEdbc)

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSqlQueryEdbc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdSqlQueryEdbc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CdSqlQueryEdbc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDBCDOC_H__73AD8B10_9005_4334_AAFA_9D7187B0F6F7__INCLUDED_)
