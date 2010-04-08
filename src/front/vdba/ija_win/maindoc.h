/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : maindoc.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
**
**/

#if !defined(AFX_MAINDOC_H__DA2AAD9C_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_MAINDOC_H__DA2AAD9C_AF05_11D3_A322_00C04F1F754A__INCLUDED_

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

	// Implementation
public:
	virtual ~CdMainDoc();
	CString m_strFormat;
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

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

#endif // !defined(AFX_MAINDOC_H__DA2AAD9C_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
