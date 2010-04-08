/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dcdtdiff.h : header file
**    Project  : INGRESII/VDDA (vsda.ocx)
**    Author   : UK Sotheavut
**    Purpose  : The Document Data for the property page (Frame/View/Doc)
**               (CfDifferenceDetail/CvDifferenceDetail)
**
** History:
**
** 10-Dec-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined(AFX_DCDTDIFF_H__6F33C6A2_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
#define AFX_DCDTDIFF_H__6F33C6A2_0B84_11D7_87F9_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CdDifferenceDetail : public CDocument
{
	DECLARE_SERIAL(CdDifferenceDetail)
public:
	CdDifferenceDetail();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdDifferenceDetail)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdDifferenceDetail();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdDifferenceDetail)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCDTDIFF_H__6F33C6A2_0B84_11D7_87F9_00C04F1F754A__INCLUDED_)
