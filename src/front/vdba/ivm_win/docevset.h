/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : docevset.h , Header File                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Document for Event Setting Frame                                      //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/

#if !defined(AFX_DOCEVSET_H__0A2EF7D6_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_DOCEVSET_H__0A2EF7D6_129C_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CdEventSetting : public CDocument
{
public:
	CdEventSetting();
	DECLARE_DYNCREATE(CdEventSetting)


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdEventSetting)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CdEventSetting();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdEventSetting)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOCEVSET_H__0A2EF7D6_129C_11D3_A2EE_00609725DDAF__INCLUDED_)
