/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DocVNode.h, Header file    (Document for MDI Child Frame)             //
//                                          CFrmVNodeMDIChild                          //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when            //
//               it is not a Docking View.                                             //
****************************************************************************************/
#ifndef DOCVNODE_HEADER
#define DOCVNODE_HEADER
class CDocVNodeMDIChild : public CDocument
{
private:
  // serialization
  BOOL            m_bLoaded;
  WINDOWPLACEMENT m_wplj;

protected:
    CDocVNodeMDIChild();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CDocVNodeMDIChild)

public:
  // serialization
  BOOL  IsLoadedDoc()           { return m_bLoaded; }
  WINDOWPLACEMENT *GetWPLJ()    { return &m_wplj; }


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDocVNodeMDIChild)
    public:
    virtual void Serialize(CArchive& ar);   // overridden for document i/o
    protected:
    virtual BOOL OnNewDocument();
    //}}AFX_VIRTUAL

public:
    virtual ~CDocVNodeMDIChild();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CDocVNodeMDIChild)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif
