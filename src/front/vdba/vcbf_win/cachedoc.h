/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cachedoc.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Document for Frame/View the page of DBMS Cache                        //
****************************************************************************************/

#if !defined (CACHEDOC_HEADER)
#define CACHEDOC_HEADER

class CdCacheDoc : public CDocument
{
public:
	CdCacheDoc();           
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdCacheDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	virtual ~CdCacheDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdCacheDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
