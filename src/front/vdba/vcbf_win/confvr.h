/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : confvr.h, header file                                                 //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : View (right-pane of the page Configure)                               //
//               See the CONCEPT.DOC for more detail                                   //
****************************************************************************************/


#include "crightdg.h"
#include "cbfdisp.h"

class CConfViewRight : public CView
{
protected:
	CConfViewRight();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CConfViewRight)

	//
	// Operations
	//
public:
	CDialog* GetCConfRightDlg(){return m_pConfigRightDlg;}
	BOOL DisplayPage (CuPageInformation* pPageInfo);

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfViewRight)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CConfRightDlg* m_pConfigRightDlg;

	virtual ~CConfViewRight();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CConfViewRight)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
