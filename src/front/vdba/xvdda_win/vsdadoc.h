/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdadoc.h : interface of the CdSda class
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame/View/Doc Architecture (doc)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
*/


#if !defined(AFX_VSDADOC_H__2F5E535B_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
#define AFX_VSDADOC_H__2F5E535B_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CdSda : public CDocument
{
protected: 
	CdSda();
	DECLARE_DYNCREATE(CdSda)

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdSda)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdSda();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CdSda)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDADOC_H__2F5E535B_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
