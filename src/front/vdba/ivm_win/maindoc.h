/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : maindoc.h , Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Main document for SDI                                                 //
****************************************************************************************/
/* History:
* --------
* uk$so01: (14-Dec-1998 created)
*
*
*/

#if !defined(AFX_MAINDOC_H__63A2E04B_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_MAINDOC_H__63A2E04B_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CdMainDoc : public CDocument
{
protected: // create from serialization only
	CdMainDoc();
	DECLARE_DYNCREATE(CdMainDoc)

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdMainDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	void LoadEvents();
	void StoreEvents();
	HWND GetLeftViewHandle() {return m_hwndLeftView;}
	HWND GetRightViewHandle(){return m_hwndRightView;}

	//
	// Implementation
public:
	virtual ~CdMainDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	HWND m_hwndLeftView;
	HWND m_hwndRightView;
	HWND m_hwndMainFrame;
	HWND m_hwndMainDlg;



protected:
	CString m_strArchiveFile;
	//
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

#endif // !defined(AFX_MAINDOC_H__63A2E04B_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
