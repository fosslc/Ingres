/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : vcbfdoc.h, header file                                                //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : App Main Doc                                                          //
****************************************************************************************/
#if !defined (VCBFDOC_HEADER)
#define VCBFDOC_HEADER

class CVcbfDoc : public CDocument
{
protected: // create from serialization only
	CVcbfDoc();
	DECLARE_DYNCREATE(CVcbfDoc)

// Attributes
public:

// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVcbfDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CVcbfDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CVcbfDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
