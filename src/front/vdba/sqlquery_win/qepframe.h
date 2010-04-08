/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepframe.h, Header file
**    Project  : CA-OpenIngres Visual DBA. 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The Frame for drawing the QEP's Tree.
**
** History:
**
** xx-Oct97 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QEPFRAME_H__DC4E108A_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
#define AFX_QEPFRAME_H__DC4E108A_46D7_11D1_A20A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CvQueryExecutionPlanView;
class CdQueryExecutionPlanDoc;
class CfQueryExecutionPlanFrame : public CFrameWnd
{
public:
	CfQueryExecutionPlanFrame();
	CfQueryExecutionPlanFrame(CdQueryExecutionPlanDoc* pDoc);
	void SetMode (BOOL bPreview, POSITION posSequence);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfQueryExecutionPlanFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	CvQueryExecutionPlanView* m_pQueryExecutionPlanView;

	virtual ~CfQueryExecutionPlanFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//
	// Generated message map functions
protected:
	CdQueryExecutionPlanDoc* m_pDoc;
	//{{AFX_MSG(CfQueryExecutionPlanFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QEPFRAME_H__DC4E108A_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
