/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : repcodoc.h, header file  
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document for Frame/View the page of Replication Page
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
*/

#if !defined (FRAME_REPLICATION_PAGE_COLLISION_DOC_HEADER)
#define FRAME_REPLICATION_PAGE_COLLISION_DOC_HEADER
#include "collidta.h"

class CdReplicationPageCollisionDoc: public CDocument
{
public:
	CdReplicationPageCollisionDoc ();

	void SetCollisionData(CaCollision* pData){m_pCollisionData = pData;}
	CaCollision* GetCollisionData(){return m_pCollisionData;}
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdReplicationPageCollisionDoc )
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL
	//
	// Implementation
protected:
	CaCollision* m_pCollisionData;

public:
	virtual ~CdReplicationPageCollisionDoc ();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CdReplicationPageCollisionDoc )
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
