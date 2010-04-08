/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : maindoc.h, Header File                                                //
//                                                                                     //
//                                                                                     //
//    Project  : Virtual Node Manager.                                                 //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Main Document (Frame, View, Doc design). It contains the Tree Data    //
****************************************************************************************/
#if !defined(AFX_MAINDOC_H__2D0C26CC_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_MAINDOC_H__2D0C26CC_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "treenode.h"

class CdMainDoc : public CDocument
{
protected: // create from serialization only
	CdMainDoc();
	DECLARE_DYNCREATE(CdMainDoc)

public:
	CaTreeNodeFolder& GetDataVNode(){return m_dataVNode;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdMainDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdMainDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CaTreeNodeFolder m_dataVNode;

	// Generated message map functions
protected:
	//{{AFX_MSG(CdMainDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDOC_H__2D0C26CC_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
